#ifndef __MENU_RENDER_H__
#define __MENU_RENDER_H__

#include "ssd1306.h"
#include "DataManager.h"
#include "BitManager.h"
#include "esp_log.h"
#include "string.h"
#include "WifiManager.h"


#define TAG_SCREEN_MANAGER "SCREEN_MANAGER"

void ScreenManagerInit(ssd1306_handle_t *_oled);
void MenuRender(menu_list_t *menu, int8_t *selected,objectInfoManager_t *objectInfo);
void ScreenWifiCallback(DataManager_t *data);
#endif  