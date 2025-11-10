#include "MenuButton.h"

int last_up = 1, last_down = 1, last_sel = 1, last_back = 1;
int up = 0, down = 0, sel = 0, back = 0;

void MenuButtonInit(void) {
  gpio_config_t io_conf = {.pin_bit_mask =
                               (1ULL << BTN_UP_GPIO) | (1ULL << BTN_DOWN_GPIO) |
                               (1ULL << BTN_SEL_GPIO) | (1ULL << BTN_BACK_GPIO),
                           .mode = GPIO_MODE_INPUT,
                           .pull_up_en = GPIO_PULLUP_ENABLE,
                           .pull_down_en = GPIO_PULLDOWN_DISABLE,
                           .intr_type = GPIO_INTR_DISABLE};
  gpio_config(&io_conf);
  ESP_LOGI(TAG_MENU_BUTTON, "MenuButtonInit done");
}

// void NavigationPointer(ssd1306_handle_t oled, uint8_t value,
//                        uint8_t prev_value) {
//   ssd1306_clear_region(oled, 0, prev_value * 12, 18, 15);
//   // Draw new arrow
//   ssd1306_draw_bitmap(oled, 0, value * 12,
//                       (const uint8_t *)image_Release_arrow_bits, 18, 15);
//   ssd1306_refresh_gram(oled);
// }

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
  } else if (down && !last_down) {
    result = BTN_DOWN;
  } else if (sel && !last_sel) {
    result = BTN_SEL;
  } else if (back && !last_back) {
    result = BTN_BACK;
  }
  last_up = up;
  last_down = down;
  last_sel = sel;
  last_back = back;
  return result;
}