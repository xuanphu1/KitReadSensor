#ifndef __SENSOR_CONFIG_H__
#define __SENSOR_CONFIG_H__

#include "SensorTypes.h"
#include "bme280.h"
#include "esp_log.h"
#include "ErrorCodes.h"

/* -------------------- Sensor Config Functions -------------------- */
// Các hàm cấu hình và quản lý sensor (wrapper cho các driver)

system_err_t SensorConfigInit(void);
system_err_t SensorConfigRead(SensorData_t *data);
system_err_t SensorConfigDeinit(void);

/* -------------------- BME280 Driver Wrapper Functions -------------------- */
// Các hàm wrapper cho BME280 driver

system_err_t bme280Initialize(void);
system_err_t bme280ReadData(SensorData_t *data);
system_err_t bme280Deinitialize(void);


#endif /* __SENSOR_CONFIG_H__ */


