#include "main.h"
#include "Common.h"
#include "LedRGB.h"
#include "nvs_flash.h"
#include "BatteryManager.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/uart.h"
#include "sdkconfig.h"
#include "ds3231.h"
#include "SD_Card.h"
#include <time.h>

/**
 * @brief Khởi tạo chân Trigger (output), Echo (input) và các chân Analog (ADC).
 * Dùng CONFIG từ Kconfig: IO_1_PORT_2 = trigger, IO_2_PORT_2 = echo,
 * IO_1_PORT_1, IO_3_PORT_2, IO_1_PORT_3 = analog.
 */
static esp_err_t InitGPIO(void) {
  esp_err_t ret;

  // 1. Trigger (output)
  gpio_config_t trigger_conf = {
    .pin_bit_mask = (1ULL << CONFIG_IO_1_PORT_2),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ret = gpio_config(&trigger_conf);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_MAIN, "Failed to config trigger GPIO %d: %s", CONFIG_IO_1_PORT_2, esp_err_to_name(ret));
    return ret;
  }
  gpio_set_level((gpio_num_t)CONFIG_IO_1_PORT_2, 0);
  ESP_LOGI(TAG_MAIN, "Trigger pin GPIO %d configured as OUTPUT", CONFIG_IO_1_PORT_2);

  // 2. Echo (input)
  gpio_config_t echo_conf = {
    .pin_bit_mask = (1ULL << CONFIG_IO_2_PORT_2),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  ret = gpio_config(&echo_conf);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_MAIN, "Failed to config echo GPIO %d: %s", CONFIG_IO_2_PORT_2, esp_err_to_name(ret));
    return ret;
  }
  ESP_LOGI(TAG_MAIN, "Echo pin GPIO %d configured as INPUT", CONFIG_IO_2_PORT_2);

  // 3. Chân analog: ESP32 = ADC1 GPIO 32,33,34,35,36,39 | ESP32-C6 = ADC1 GPIO 0–6
#if CONFIG_IDF_TARGET_ESP32
  ret = adc1_config_width(ADC_WIDTH_BIT_12);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_MAIN, "Failed to config ADC width: %s", esp_err_to_name(ret));
    return ret;
  }
  const int analog_gpios[] = { CONFIG_IO_1_PORT_1, CONFIG_IO_3_PORT_2, CONFIG_IO_1_PORT_3 };
  const int n_analog = sizeof(analog_gpios) / sizeof(analog_gpios[0]);
  for (int i = 0; i < n_analog; i++) {
    int gpio = analog_gpios[i];
    adc1_channel_t ch;
    switch (gpio) {
      case 32: ch = ADC1_CHANNEL_4; break;
      case 33: ch = ADC1_CHANNEL_5; break;
      case 34: ch = ADC1_CHANNEL_6; break;
      case 35: ch = ADC1_CHANNEL_7; break;
      case 36: ch = ADC1_CHANNEL_0; break;
      case 39: ch = ADC1_CHANNEL_3; break;
      default:
        ESP_LOGW(TAG_MAIN, "GPIO %d is not ADC1 on ESP32, skip", gpio);
        continue;
    }
    ret = adc1_config_channel_atten(ch, ADC_ATTEN_DB_11);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG_MAIN, "Failed to config ADC GPIO %d: %s", gpio, esp_err_to_name(ret));
      return ret;
    }
    ESP_LOGI(TAG_MAIN, "Analog GPIO %d configured as ADC1", gpio);
  }
#elif CONFIG_IDF_TARGET_ESP32C6
  // ESP32-C6 có ADC1 trên GPIO 0–6. Cấu hình các chân analog từ Kconfig làm input
  // (đọc ADC thực tế dùng API mới: esp_adc/adc_oneshot.h hoặc adc_continuous.h).
  const int analog_gpios[] = { CONFIG_IO_1_PORT_1, CONFIG_IO_3_PORT_2, CONFIG_IO_1_PORT_3 };
  const int n_analog = sizeof(analog_gpios) / sizeof(analog_gpios[0]);
  for (int i = 0; i < n_analog; i++) {
    int gpio = analog_gpios[i];
    if (gpio < 0 || gpio > 6) {
      ESP_LOGW(TAG_MAIN, "ESP32-C6 ADC1 only GPIO 0-6, skip GPIO %d", gpio);
      continue;
    }
    gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << gpio),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG_MAIN, "Failed to config analog GPIO %d: %s", gpio, esp_err_to_name(ret));
      return ret;
    }
    ESP_LOGI(TAG_MAIN, "Analog GPIO %d (ADC1) configured as INPUT", gpio);
  }
#endif

  ESP_LOGI(TAG_MAIN, "Trigger, Echo and Analog pins initialized");
  return ESP_OK;
}

// static void rtc_sd_log_task(void *pvParameters) {
//   (void)pvParameters;
//   esp_err_t ret;

//   // Khởi tạo descriptor cho DS3231 dùng bus I2C chung
//   i2c_dev_t rtc;
//   memset(&rtc, 0, sizeof(rtc));
//   ret = ds3231_init_desc(&rtc, CONFIG_I2CDEV_COMMON_PORT,
//                          CONFIG_I2CDEV_COMMON_SDA, CONFIG_I2CDEV_COMMON_SCL);
//   if (ret != ESP_OK) {
//     ESP_LOGE(TAG_MAIN, "DS3231 init failed: %s", esp_err_to_name(ret));
//     vTaskDelete(NULL);
//   }

//   // Khởi tạo SD Card
//   ret = initSDCard();
//   if (ret != ESP_OK) {
//     ESP_LOGE(TAG_MAIN, "SD card init failed: %s", esp_err_to_name(ret));
//     vTaskDelete(NULL);
//   }

//   while (1) {
//     struct tm now = {0};
//     char time_str[40];
//     char line[80];

//     ret = ds3231_get_time(&rtc, &now);
//     if (ret == ESP_OK) {
//       ds3231_get_time_str(&now, time_str, sizeof(time_str));
//       snprintf(line, sizeof(line), "%s", time_str);

//       // Ghi thêm 1 dòng vào file log trên SD
//       esp_err_t wret = writeFinalFileSD_Card("rtc_log.txt", line);
//       if (wret != ESP_OK) {
//         ESP_LOGW(TAG_MAIN, "Failed to write RTC log to SD: %s",
//                  esp_err_to_name(wret));
//       }

//       // In log ra UART để kiểm tra
//       ESP_LOGI(TAG_MAIN, "RTC time: %s", time_str);
//     } else {
//       ESP_LOGW(TAG_MAIN, "DS3231 read failed: %s", esp_err_to_name(ret));
//     }

//     vTaskDelay(pdMS_TO_TICKS(5000)); // 5s ghi 1 lần để test
//   }
// }

void app_main(void) {

  // Khởi tạo trigger, echo và các chân analog trước (theo Kconfig)
  esp_err_t pin_ret = InitGPIO();
  if (pin_ret != ESP_OK) {
    ESP_LOGE(TAG_MAIN, "Failed to init trigger/echo/analog pins: %s", esp_err_to_name(pin_ret));
  }

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  
  // Khởi tạo LED RGB
  esp_err_t led_ret = LedRGB_Init();
  if (led_ret != ESP_OK) {
    ESP_LOGW(TAG_MAIN, "Failed to initialize LED RGB: %s", esp_err_to_name(led_ret));
  }
  
  ButtonManagerInit();
  ESP_ERROR_CHECK(i2cInitDevCommon());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  MainScreen = ssd1306_create(I2C_NUM_0, SSD1306_I2C_ADDRESS);
#pragma GCC diagnostic pop
  if (MainScreen == NULL) {
    ESP_LOGE(TAG_MAIN, "Failed to create SSD1306 handle");
    vTaskDelay(portMAX_DELAY);
  }
  ScreenManagerInit(&MainScreen);
  /* Đảm bảo chưa chọn cảm biến nào -> menu hiển thị "Port 1", "Port 2", "Port 3" chứ không "Port 1 - BME280" do zero-init */
  for (int i = 0; i < NUM_PORTS; i++) {
    DataManager.selectedSensor[i] = SENSOR_NONE;
  }
  MenuSystemInit(&DataManager);
  
  // Khởi tạo Battery Manager
  // esp_err_t battery_ret = BatteryManager_Init();
  // if (battery_ret != ESP_OK) {
  //   ESP_LOGW(TAG_MAIN, "Failed to initialize Battery Manager: %s", esp_err_to_name(battery_ret));
  // } else {
  //   // Khởi động FreeRTOS task để đọc pin định kỳ
  //   BatteryManager_StartTask(&DataManager);
  // }
  
  xTaskCreate(wifi_init_sta, "wifi_init_sta", 4096, &DataManager, 5, NULL);
  xTaskCreate(MenuNavigation_Task, "MenuNavigation_Task", 4096,
              &DataManager, 5, NULL);

  // Task test: đọc thời gian DS3231 và ghi log vào SD card
  //xTaskCreate(rtc_sd_log_task, "rtc_sd_log_task", 4096, NULL, 5, NULL);
  while (1) {
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

