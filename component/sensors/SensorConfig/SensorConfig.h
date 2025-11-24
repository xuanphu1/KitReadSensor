#ifndef __SENSOR_CONFIG_H__
#define __SENSOR_CONFIG_H__

#include "SensorTypes.h"
#include "bme280.h"
#include "esp_log.h"

/* -------------------- Sensor Config Functions -------------------- */
// Các hàm cấu hình và quản lý sensor (wrapper cho các driver)

void SensorConfigInit(void);
void SensorConfigRead(SensorData_t *data);
void SensorConfigDeinit(void);

/* -------------------- BME280 Driver Wrapper Functions -------------------- */
// Các hàm wrapper cho BME280 driver

void sensor_bme280_init(void);
void sensor_bme280_read(SensorData_t *data);
void sensor_bme280_deinit(void);

#endif /* __SENSOR_CONFIG_H__ */


