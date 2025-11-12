#include "ScreenManager.h"
#include "DataManager.h"
ssd1306_handle_t oled = NULL;

static void ssd1306_draw_menu_item(menu_item_t *item, int index, int selected,
                                   int offset) {
  int y = index * 12 + offset;
  if (selected) {
    ssd1306_draw_string(oled, 0, y, (uint8_t *)">", 12, 1);
  }
  ssd1306_draw_string(oled, 12, y, (uint8_t *)item->name, 12, 1);
}

static void initUIState(void);

void ScreenManagerInit(ssd1306_handle_t *_oled) {
  oled = *_oled;
  initUIState();
  vTaskDelay(2000 / portTICK_PERIOD_MS);
}

static void initUIState(void) {
  ssd1306_clear_screen(oled, 0x00);
  ssd1306_draw_bitmap(oled, 40, -18,
                      (uint8_t *)imageManager[OBJECT_DIFFERENT][0], 48, 71);
  ssd1306_draw_string(oled, 16, 52, (const uint8_t *)"Designed by MrKoi", 12,
                      1);
  ssd1306_draw_string(oled, 54, 37, (const uint8_t *)"MRS", 16, 1);
  ssd1306_refresh_gram(oled);
}

void MenuRender(menu_list_t *menu, int8_t *selected,
                objectInfoManager_t *objectInfo) {
  ssd1306_clear_screen(oled, 0);

  uint32_t offset = 0;
  if (menu == NULL) {
    ESP_LOGW(TAG_SCREEN_MANAGER, "MenuRender: menu is NULL");
  }
  if (menu->image.image != NULL) {
    switch (menu->object) {
    case OBJECT_WIFI:
      update_wifi_status(&(objectInfo->wifiInfo));

      ssd1306_draw_bitmap(
          oled, 55, 0,
          (uint8_t *)
              menu->image.image[menu->object][objectInfo->wifiInfo.wifiStatus],
          menu->image.width, menu->image.height);
      break;
    case OBJECT_BATTERY:
      ssd1306_draw_bitmap(
          oled, 55, 0,
          (uint8_t *)menu->image
              .image[menu->object][objectInfo->batteryInfo.batteryLevel % 17],
          menu->image.width, menu->image.height);
      break;
    case OBJECT_DIFFERENT:
      break;
    case OBJECT_SENSOR:
      break;
    case OBJECT_INFORMATION:
      break;
    default:
      break;
    }
    offset += menu->image.height;
  }
  if (menu->text.text != NULL) {
    bool extraText = false;
    switch (menu->object) {
    case OBJECT_WIFI: {
      unsigned int textLength = strlen(
          menu->text.text[menu->object][objectInfo->wifiInfo.wifiStatus]);
      ssd1306_draw_string(
          oled, 0, offset,
          (uint8_t *)
              menu->text.text[menu->object][objectInfo->wifiInfo.wifiStatus],
          menu->text.size, 1);
      if (objectInfo->wifiInfo.wifiStatus == CONNECTED) {
        char wifi_info_text[64];
        snprintf(wifi_info_text, sizeof(wifi_info_text), "%s%s",
                 objectInfo->wifiInfo.wifiName,
                 menu->text.text[menu->object][2]);

        ssd1306_draw_string(oled, textLength * (menu->text.size / 2), offset,
                            (uint8_t *)wifi_info_text, menu->text.size, 1);

        textLength += strlen(wifi_info_text);

        // Khi cần clear:
        memset(wifi_info_text, 0, sizeof(wifi_info_text));
      }
      if (textLength > MAX_TEXT_LENGTH)
        extraText = true;
    } break;
    case OBJECT_BATTERY:
      ssd1306_draw_string(
          oled, 55, offset,
          (uint8_t *)menu->text
              .text[menu->object][objectInfo->batteryInfo.batteryLevel % 17],
          menu->text.size, 1);
      if (strlen(menu->text.text[menu->object]
                                [objectInfo->batteryInfo.batteryLevel % 17]) >
          MAX_TEXT_LENGTH)
        extraText = true;
      break;
    case OBJECT_DIFFERENT:
      break;
    case OBJECT_SENSOR:
      break;
    case OBJECT_INFORMATION:
      break;
    default:
      break;
    }
    if (extraText) {
      offset += menu->text.size * 2;
    } else {
      offset += menu->text.size;
    }
  }
  int start_index = 0;
  int end_index = menu->count;

  if (menu->count > MAX_VISIBLE_ITEMS) {
    // Nếu có nhiều hơn 4 items, áp dụng pagination
    if (*selected >= MAX_VISIBLE_ITEMS) {
      // Khi selected >= 4, hiển thị từ selected trở đi (selected, selected+1,
      // selected+2, selected+3)
      start_index = *selected;
      end_index = *selected + MAX_VISIBLE_ITEMS;

      // Đảm bảo không vượt quá count
      if (end_index > menu->count) {
        end_index = menu->count;
      }
    } else {
      // Khi selected < 4, hiển thị từ 0 đến 3
      start_index = 0;
      end_index = MAX_VISIBLE_ITEMS;
    }
  }

  // Hiển thị items trong phạm vi đã tính toán
  int display_index = 0;
  for (int i = start_index; i < end_index; i++) {
    ssd1306_draw_menu_item(&menu->items[i], display_index, (i == *selected),
                           offset);
    display_index++;
  }
  ssd1306_refresh_gram(oled);
}

void ScreenWifiConnecting(DataManager_t *data) {
  static uint8_t dot_count = 0;
  const char *base_message = "Connecting to WiFi";
  char display_buffer[30];
  if (data->objectInfo.wifiInfo.wifiStatus == DISCONNECTED ||
      data->objectInfo.wifiInfo.wifiStatus == ERROR) {
    strcpy(display_buffer, base_message);
    for (int i = 0; i < (dot_count % 4); i++) {
      strcat(display_buffer, ".");
    }
    dot_count++;
    ssd1306_clear_screen(oled, 0);
    ssd1306_draw_string(oled, 0, 0, (uint8_t *)display_buffer, 12, 1);
    ssd1306_refresh_gram(oled);
  } else {
    ESP_LOGW(TAG_SCREEN_MANAGER,
             "ScreenWifiCallback: wifi status is not DISCONNECTED or ERROR");
  }
}

void ScreenShowMessage(int index) {
  ssd1306_clear_screen(oled, 0);
  ssd1306_draw_string(oled, 0, 0, (uint8_t *)MessageText[index], 12, 1);
  ssd1306_refresh_gram(oled);
}

void ScreenShowDataSensor(const char **field_names, const float *values,
                          const char **units, size_t count) {
  if (field_names == NULL || values == NULL || units == NULL || count == 0) {
    ESP_LOGW(TAG_SCREEN_MANAGER,
             "ScreenShowDataSensor: invalid arguments (fields=%p, values=%p, "
             "units=%p, count=%u)",
             field_names, values, units, (unsigned)count);
    return;
  }

  static size_t current_index = 0;

  // Hiển thị lần lượt từng trường, mỗi trường 300ms
  size_t iterations = count < 3 ? count : 3; // chỉ hiển thị tối đa 3 trường
  for (size_t step = 0; step < iterations; ++step) {
    size_t idx = current_index % count;

    const char *name = field_names[idx] ? field_names[idx] : "Field";
    const char *unit = units[idx] ? units[idx] : "";
    float value = values[idx];

    ssd1306_clear_screen(oled, 0);

    // Tên trường (12px) - căn giữa
    {
      unsigned int name_px = strlen(name) * (12 / 2);
      int name_x = (int)((SSD1306_WIDTH - name_px) / 2);
      if (name_x < 0)
        name_x = 0;
      ssd1306_draw_string(oled, name_x, 0, (uint8_t *)name, 12, 1);
    }

    // Giá trị: vẽ lớn hơn bằng font 32x16 cho các ký tự số, dấu '.' và '-' vẽ
    // font 12
    char value_buf[24];
    snprintf(value_buf, sizeof(value_buf), "%.2f", value);
    // Tính chiều rộng để căn giữa (ước lượng: số -> 16px, '.'/'-' -> 6px)
    int total_px = 0;
    for (const char *p = value_buf; *p; ++p) {
      if (*p >= '0' && *p <= '9')
        total_px += 16;
      else
        total_px += 6;
    }
    int base_x = (SSD1306_WIDTH - total_px) / 2;
    if (base_x < 0)
      base_x = 0;

    int cursor_x = base_x;
    int cursor_y = 12; // bắt đầu dưới dòng tên trường
    for (const char *p = value_buf; *p; ++p) {
      if (*p >= '0' && *p <= '9') {
        ssd1306_draw_3216char(oled, (uint8_t)cursor_x, (uint8_t)cursor_y,
                              (uint8_t)*p);
        cursor_x += 16;
      } else {
        // vẽ nhỏ cho '.' hoặc '-'
        char tmp[2] = {*p, '\0'};
        ssd1306_draw_string(oled, (uint8_t)cursor_x, (uint8_t)(cursor_y + 8),
                            (uint8_t *)tmp, 12, 1);
        cursor_x += 6;
      }
    }

    // Đơn vị (12px) - căn giữa
    if (unit[0] != '\0') {
      unsigned int unit_px = (unsigned int)(strlen(unit) * (12 / 2));
      int unit_x = (int)((SSD1306_WIDTH - unit_px) / 2);
      if (unit_x < 0)
        unit_x = 0;
      // đặt đơn vị dưới phần giá trị lớn
      ssd1306_draw_string(oled, unit_x, cursor_y + 32, (uint8_t *)unit, 12, 1);
    }

    ssd1306_refresh_gram(oled);
    vTaskDelay(pdMS_TO_TICKS(300));
    current_index = (current_index + 1) % count;
  }
}