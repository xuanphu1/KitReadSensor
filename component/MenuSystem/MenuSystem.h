#ifndef __MENU_H__
#define __MENU_H__

#include "esp_log.h"
#include "ssd1306.h"
#include "DataManager.h"
#include "string.h"
#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MenuButton.h"
#include <sys/param.h>
#include "FunctionManager.h"
#include "WifiManager.h"
#include "BitManager.h"
#include "ScreenManager.h"

#define TAG_MENU_SYSTEM "MENU_SYSTEM"


void MenuSystemInit();
void NavigationScreen_Task(void *pvParameter);
void ReadSensor_Task(void *pvParameter);
#endif
