#ifndef __SENSOR_SYSTEM_H__
#define __SENSOR_SYSTEM_H__

#include "DataManager.h"
#include "bme280.h"

void SensorSystemInit(void);
void SensorSystemRead(SensorData_t *data);
void SensorSystemDeinit(void);


// BME280 wrapper functions

void bme280Init(void);
void bme280Read(SensorData_t *data);
void bme280Deinit(void);


#endif // __SENSOR_SYSTEM_H__