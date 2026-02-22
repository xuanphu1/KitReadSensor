#include "ButtonManager.h"
#include "LedRGB.h"

// Biến lưu trạng thái trước đó của các nút
static int last_up = 1, last_down = 1, last_sel = 1, last_back = 1;
// Biến lưu trạng thái hiện tại của các nút
static int up = 0, down = 0, sel = 0, back = 0;

void ButtonManagerInit(void) {
  gpio_config_t io_conf = {.pin_bit_mask =
                               (1ULL << BTN_UP_GPIO) | (1ULL << BTN_DOWN_GPIO) |
                               (1ULL << BTN_SEL_GPIO) | (1ULL << BTN_BACK_GPIO),
                           .mode = GPIO_MODE_INPUT,
                           .pull_up_en = GPIO_PULLUP_DISABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);
  ESP_LOGI(TAG_BUTTON_MANAGER, "ButtonManagerInit done - 4 buttons initialized");
}

static int read_button_once(gpio_num_t pin) {
  int level = gpio_get_level(pin);
  return level == 0; // active low
}

static int read_button_debounced(gpio_num_t pin) {
  if (!read_button_once(pin))
    return 0;
  vTaskDelay(20 / portTICK_PERIOD_MS);
  return read_button_once(pin);
}

button_type_t ReadButtonStatus(void) {
  button_type_t result = BTN_NONE;
  
  // Đọc trạng thái của cả 4 nút
  up = read_button_debounced(BTN_UP_GPIO);
  down = read_button_debounced(BTN_DOWN_GPIO);
  sel = read_button_debounced(BTN_SEL_GPIO);
  back = read_button_debounced(BTN_BACK_GPIO);

  // Xử lý BTN_UP - phát hiện cạnh xuống (từ không nhấn -> nhấn)
  if (up && !last_up) {
    result = BTN_UP;
    LedRGB_SetButtonColor(BTN_UP);
  }
  
  // Xử lý BTN_DOWN - phát hiện cạnh xuống
  if (down && !last_down) {
    result = BTN_DOWN;
    LedRGB_SetButtonColor(BTN_DOWN);
  }
  
  // Xử lý BTN_SEL - phát hiện cạnh xuống
  if (sel && !last_sel) {
    result = BTN_SEL;
    LedRGB_SetButtonColor(BTN_SEL);
  }
  
  // Xử lý BTN_BACK - phát hiện cạnh xuống
  if (back && !last_back) {
    result = BTN_BACK;
    LedRGB_SetButtonColor(BTN_BACK);
  }
  
  // Cập nhật trạng thái trước đó
  last_up = up;
  last_down = down;
  last_sel = sel;
  last_back = back;
  
  return result;
}

