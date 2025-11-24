#include "ButtonManager.h"
#include "LedRGB.h"

int last_up = 1, last_down = 1, last_sel = 1, last_back = 1;
int up = 0, down = 0, sel = 0, back = 0;

void ButtonManagerInit(void) {
  gpio_config_t io_conf = {.pin_bit_mask =
                               (1ULL << BTN_UP_GPIO) | (1ULL << BTN_DOWN_GPIO) |
                               (1ULL << BTN_SEL_GPIO) | (1ULL << BTN_BACK_GPIO),
                           .mode = GPIO_MODE_INPUT,
                           .pull_up_en = GPIO_PULLUP_ENABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);
  ESP_LOGI(TAG_BUTTON_MANAGER, "ButtonManagerInit done");
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
  up = read_button_debounced(BTN_UP_GPIO);
  down = read_button_debounced(BTN_DOWN_GPIO);
  sel = read_button_debounced(BTN_SEL_GPIO);
  back = read_button_debounced(BTN_BACK_GPIO);

  if (up && !last_up) {
    result = BTN_UP;
    LedRGB_SetButtonColor(BTN_UP);
  } else if (down && !last_down) {
    result = BTN_DOWN;
    LedRGB_SetButtonColor(BTN_DOWN);
  } else if (sel && !last_sel) {
    result = BTN_SEL;
    LedRGB_SetButtonColor(BTN_SEL);
  } else if (back && !last_back) {
    result = BTN_BACK;
    LedRGB_SetButtonColor(BTN_BACK);
  }
  last_up = up;
  last_down = down;
  last_sel = sel;
  last_back = back;
  return result;
}

