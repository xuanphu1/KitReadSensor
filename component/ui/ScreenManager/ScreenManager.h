#ifndef __MENU_RENDER_H__
#define __MENU_RENDER_H__

#include "BitManager.h"
#include "Common.h"
#include "ErrorCodes.h"
#include "WifiManager.h"
#include "esp_log.h"
#include "ssd1306.h"
#include "string.h"

#define TAG_SCREEN_MANAGER "SCREEN_MANAGER"

system_err_t ScreenManagerInit(ssd1306_handle_t *_oled);
system_err_t MenuRender(menu_list_t *menu, int8_t *selected,
                        objectInfoManager_t *objectInfo);
system_err_t SensorRender(PortId_t port, SensorData_t *data);
system_err_t ScreenWifiConnecting(DataManager_t *data);
system_err_t ScreenShowMessage(Message_t message);
/** Hiển thị nhiều dòng text lên OLED (vd màn Information). lines[0..n-1], mỗi
 * dòng cách nhau 12px. */
system_err_t ScreenShowInformation(const char **lines, size_t n_lines);
system_err_t ScreenShowDataSensor(const char **field_names, const float *values,
                                  const char **units, size_t count);
system_err_t ScreenShowMeshInformation(DataManager_t *data);
#endif