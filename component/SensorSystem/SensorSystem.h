#ifndef __SENSOR_SYSTEM_H__
#define __SENSOR_SYSTEM_H__

#include "DataManager.h"

void SensorSystemInit(void);
void SensorSystemRead(SensorData_t *data);
void SensorSystemDeinit(void);

#endif // __SENSOR_SYSTEM_H__