#ifndef __FUNCTION_MANAGER_H__
#define __FUNCTION_MANAGER_H__

#include "esp_log.h"
#include "string.h"
#include "ScreenManager.h"
#include <stdbool.h>
#include "Common.h"
#include "SensorConfig.h"
#include "ErrorCodes.h"
#define TAG_FUNCTION_MANAGER "FUNCTION_MANAGER"

void wifi_config_callback(void *ctx);
void information_callback(void *ctx);
void read_temperature_cb(void *ctx);
void read_humidity_cb(void *ctx);
void read_pressure_cb(void *ctx);
void read_dht22_cb(void *ctx);
void battery_status_callback(void *ctx);
void reset_all_ports_callback(void *ctx);

// Tracking lựa chọn cảm biến/port và bắt đầu đọc
void select_sensor_cb(void *ctx);
void show_data_sensor_cb(void *ctx);

#endif //