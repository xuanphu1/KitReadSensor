#ifndef __MENU_H__
#define __MENU_H__

#include "esp_log.h"
#include "ssd1306.h"
#include "Common.h"
#include "string.h"
#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ButtonManager.h"
#include <sys/param.h>
#include "FunctionManager.h"
#include "WifiManager.h"
#include "BitManager.h"
#include "ScreenManager.h"

#define TAG_MENU_SYSTEM "MENU_SYSTEM"


void MenuSystemInit(DataManager_t *data);
void MenuNavigation_Task(void *pvParameter);
void ReadSensor_Task(void *pvParameter);
/** Cập nhật tên Sensor_Menu_Items[0..2] thành "Port X" hoặc "Port X - tên cảm biến" theo data->selectedSensor. */
void MenuSystem_UpdatePortNames(DataManager_t *data);
/** Chuyển màn hình về menu Sensors và vẽ lại; selected_index là chỉ số mục được chọn (0=Port1, 1=Port2, 2=Port3). */
void MenuSystem_GoToSensorMenu(DataManager_t *data, int8_t selected_index);

// SensorSelection được cấp phát động, không cần extern declaration
// Sử dụng sensor_registry_get_count() để lấy số lượng sensor thực tế
#endif
