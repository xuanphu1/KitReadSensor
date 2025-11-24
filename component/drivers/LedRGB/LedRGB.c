#include "LedRGB.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#define TAG_LED_RGB "LED_RGB"

// Macro để lấy container struct từ member pointer (nếu chưa được định nghĩa)
#ifndef __containerof
#define __containerof(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif

#if CONFIG_ACTIVE_LED_RGB
// Cấu hình LED RGB (WS2812)
#define LED_RGB_GPIO CONFIG_LED_RGB_GPIO
#define LED_RGB_RMT_RES_HZ (10 * 1000 * 1000) // 10MHz resolution
#define LED_RGB_NUM_PIXELS 1 // Số lượng LED (thường là 1 cho ESP32-C6)

// WS2812 timing (tính bằng ticks với resolution 10MHz)
// 10MHz = 10,000,000 Hz = 100ns per tick
#define WS2812_T0H_TICKS 4   // 0 bit high time (350ns ≈ 4 ticks)
#define WS2812_T0L_TICKS 10   // 0 bit low time (1000ns = 10 ticks)
#define WS2812_T1H_TICKS 10   // 1 bit high time (1000ns = 10 ticks)
#define WS2812_T1L_TICKS 4    // 1 bit low time (350ns ≈ 4 ticks)
#define WS2812_RESET_US 80    // Reset time (80us)
static rmt_channel_handle_t led_chan = NULL;
static rmt_encoder_handle_t led_encoder = NULL;
static bool led_initialized = false;
static esp_timer_handle_t led_off_timer = NULL;

// Forward declaration
static void led_off_timer_callback(void *arg);

// RMT encoder cho WS2812
typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;
#endif

#if CONFIG_ACTIVE_LED_RGB
static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                    const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state) {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    uint8_t *bytes = (uint8_t *)primary_data;

    switch (led_encoder->state) {
    case 0: // send RGB data
        encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, bytes, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = 1; // switch to sending reset code
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    // fall-through
    case 1: // send reset code
        encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                sizeof(led_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_encoder->state = RMT_ENCODING_RESET; // back to the initial encoding session
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder) {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder) {
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}
#endif // CONFIG_ACTIVE_LED_RGB

#if CONFIG_ACTIVE_LED_RGB
// Màu sắc cho từng nút
static const struct {
    uint8_t r, g, b;
} button_colors[] = {
    [0] = {255, 0, 0},     // BTN_UP: Đỏ
    [1] = {0, 255, 0},     // BTN_DOWN: Xanh lá
    [2] = {0, 0, 255},     // BTN_SEL: Xanh dương
    [3] = {255, 255, 0},   // BTN_BACK: Vàng
};
#endif


esp_err_t LedRGB_Init(void) {
#if !CONFIG_ACTIVE_LED_RGB
    // LED RGB bị tắt trong config, trả về thành công nhưng không làm gì
    return ESP_OK;
#else
    if (led_initialized) {
        ESP_LOGW(TAG_LED_RGB, "LED RGB already initialized");
        return ESP_OK;
    }

    // Cấu hình RMT channel
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = LED_RGB_GPIO,
        .mem_block_symbols = 64,
        .resolution_hz = LED_RGB_RMT_RES_HZ,
        .trans_queue_depth = 4,
    };
    
    esp_err_t ret = rmt_new_tx_channel(&tx_chan_config, &led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_LED_RGB, "Failed to create RMT channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Tạo encoder cho bytes (8 bits)
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = WS2812_T0H_TICKS,
            .level1 = 0,
            .duration1 = WS2812_T0L_TICKS,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = WS2812_T1H_TICKS,
            .level1 = 0,
            .duration1 = WS2812_T1L_TICKS,
        },
    };
    rmt_encoder_handle_t bytes_encoder = NULL;
    ret = rmt_new_bytes_encoder(&bytes_encoder_config, &bytes_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_LED_RGB, "Failed to create bytes encoder: %s", esp_err_to_name(ret));
        rmt_del_channel(led_chan);
        led_chan = NULL;
        return ret;
    }

    // Tạo copy encoder cho reset code
    rmt_copy_encoder_config_t copy_encoder_config = {};
    rmt_encoder_handle_t copy_encoder = NULL;
    ret = rmt_new_copy_encoder(&copy_encoder_config, &copy_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_LED_RGB, "Failed to create copy encoder: %s", esp_err_to_name(ret));
        rmt_del_encoder(bytes_encoder);
        rmt_del_channel(led_chan);
        led_chan = NULL;
        return ret;
    }

    // Tạo LED strip encoder
    rmt_led_strip_encoder_t *led_encoder_obj = calloc(1, sizeof(rmt_led_strip_encoder_t));
    led_encoder_obj->base.encode = rmt_encode_led_strip;
    led_encoder_obj->base.reset = rmt_led_strip_encoder_reset;
    led_encoder_obj->base.del = rmt_del_led_strip_encoder;
    led_encoder_obj->bytes_encoder = bytes_encoder;
    led_encoder_obj->copy_encoder = copy_encoder;
    led_encoder_obj->state = 0;
    // Reset code: low for 80us (800 ticks at 10MHz)
    led_encoder_obj->reset_code = (rmt_symbol_word_t){
        .level0 = 0,
        .duration0 = 800,
        .level1 = 0,
        .duration1 = 0,
    };
    led_encoder = &led_encoder_obj->base;

    // Enable RMT channel
    ret = rmt_enable(led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_LED_RGB, "Failed to enable RMT channel: %s", esp_err_to_name(ret));
        rmt_del_encoder(led_encoder);
        rmt_del_channel(led_chan);
        led_chan = NULL;
        led_encoder = NULL;
        return ret;
    }

    // Tạo timer để tắt LED sau delay
    esp_timer_create_args_t timer_args = {
        .callback = &led_off_timer_callback,
        .name = "led_off_timer"
    };
    ret = esp_timer_create(&timer_args, &led_off_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_LED_RGB, "Failed to create LED off timer: %s", esp_err_to_name(ret));
        rmt_del_encoder(led_encoder);
        rmt_del_channel(led_chan);
        led_chan = NULL;
        led_encoder = NULL;
        return ret;
    }

    // Tắt LED ban đầu
    LedRGB_Off();
    
    led_initialized = true;
    ESP_LOGI(TAG_LED_RGB, "LED RGB initialized on GPIO %d", LED_RGB_GPIO);
    return ESP_OK;
#endif // CONFIG_ACTIVE_LED_RGB
}

esp_err_t LedRGB_SetColor(uint8_t red, uint8_t green, uint8_t blue) {
#if !CONFIG_ACTIVE_LED_RGB
    // LED RGB bị tắt, không làm gì
    return ESP_OK;
#else
    if (!led_initialized || led_chan == NULL || led_encoder == NULL) {
        ESP_LOGW(TAG_LED_RGB, "LED RGB not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // WS2812 format: GRB (Green, Red, Blue)
    uint8_t rgb_data[3] = {green, red, blue};
    
    // Tạo RMT transmit configuration
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
        .flags = {
            .eot_level = 0,
        },
    };
    
    // Gửi dữ liệu qua RMT với encoder
    esp_err_t ret = rmt_transmit(led_chan, led_encoder, rgb_data, sizeof(rgb_data), &tx_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_LED_RGB, "Failed to transmit RMT data: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
#endif // CONFIG_ACTIVE_LED_RGB
}

esp_err_t LedRGB_Off(void) {
    return LedRGB_SetColor(0, 0, 0);
}

#if CONFIG_ACTIVE_LED_RGB
// Callback để tắt LED sau delay
static void led_off_timer_callback(void *arg) {
    LedRGB_Off();
}
#endif

void LedRGB_SetButtonColor(int button_type) {
#if CONFIG_ACTIVE_LED_RGB
    // BTN_UP=0, BTN_DOWN=1, BTN_SEL=2, BTN_BACK=3, BTN_NONE=4
    if (button_type >= 0 && button_type < 4 && led_initialized) {
        // Sáng LED với màu tương ứng
        LedRGB_SetColor(button_colors[button_type].r, 
                       button_colors[button_type].g, 
                       button_colors[button_type].b);
        
        // Dừng timer cũ nếu có
        if (led_off_timer != NULL) {
            esp_timer_stop(led_off_timer);
        }
        
        // Khởi động timer để tắt LED sau 200ms
        if (led_off_timer != NULL) {
            esp_timer_start_once(led_off_timer, 50 * 1000); // 200ms = 200000 microseconds
        }
    }
#endif
}

