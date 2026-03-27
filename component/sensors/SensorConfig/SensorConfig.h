#ifndef __SENSOR_CONFIG_H__
#define __SENSOR_CONFIG_H__

#include "SensorTypes.h"
#include "bme280.h"
#include "esp_log.h"
#include "pms7003.h"
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

/* -------------------- AHT10 Driver Wrapper Functions -------------------- */
system_err_t aht10Initialize(void);
system_err_t aht10ReadData(SensorData_t *data);
system_err_t aht10Deinitialize(void);


/* -------------------- PMS7003 Driver Wrapper Functions -------------------- */
system_err_t pms7003Initialize(void);
system_err_t pms7003ReadData(SensorData_t *data);
system_err_t pms7003Deinitialize(void);


/* -------------------- MQ Analog Sensors (MQ2, MQ3, ...) -------------------- */
// Thiết lập port hiện tại (PORT_1, PORT_2, PORT_3) trước khi đọc analog
void SensorConfigSetCurrentPort(PortId_t port);
// Init analog MQ: cấu hình ADC cho các chân analog theo từng port
system_err_t mq_analog_init(void);
// Read analog MQ: đọc giá trị ADC thật của port hiện tại
system_err_t mq_analog_read(SensorData_t *data);


#endif /* __SENSOR_CONFIG_H__ */


