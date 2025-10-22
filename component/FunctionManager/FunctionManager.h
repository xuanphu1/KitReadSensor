#ifndef __FUNCTION_MANAGER_H__
#define __FUNCTION_MANAGER_H__

#include "esp_log.h"
#include "string.h"
#include "ScreenManager.h"
#include <stdbool.h>


#define TAG_FUNCTION_MANAGER "FUNCTION_MANAGER"

void wifi_config_callback(void *ctx);
void information_callback(void *ctx);
void read_temperature_cb(void *ctx);
void read_humidity_cb(void *ctx);
void read_pressure_cb(void *ctx);
void read_dht22_cb(void *ctx);
void battery_status_callback(void *ctx);

#endif //