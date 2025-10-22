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
  update_wifi_status(&(objectInfo->wifiInfo));
  uint32_t offset = 0;
  if (menu->image.image != NULL) {
    switch (menu->object) {
    case OBJECT_WIFI:
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
    case OBJECT_WIFI:
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

      break;
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
  for (int i = 0; i < menu->count; i++) {
    ssd1306_draw_menu_item(&menu->items[i], i, (i == *selected), offset);
  }
  ssd1306_refresh_gram(oled);
}

void ScreenWifiCallback(DataManager_t *data) {
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
  }
}