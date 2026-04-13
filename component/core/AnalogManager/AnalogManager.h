#ifndef ANALOG_MANAGER_H
#define ANALOG_MANAGER_H

#include <stdint.h>
#include "ErrorCodes.h"
#include "SensorTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG_ANALOG_MANAGER "ANALOG_MANAGER"

#define ANALOG_ADS_ADDR 0x48

#define ADS_REG_CONVERSION 0x00
#define ADS_REG_CONFIG 0x01

#define ADS_CFG_OS_START 0x8000
#define ADS_CFG_MUX_AIN0 0x4000
#define ADS_CFG_MUX_AIN1 0x5000
#define ADS_CFG_MUX_AIN2 0x6000
#define ADS_CFG_MUX_AIN3 0x7000
#define ADS_CFG_PGA_4V096 0x0200
#define ADS_CFG_MODE_SINGLE 0x0100
#define ADS_CFG_DR_128SPS 0x0080
#define ADS_CFG_COMP_DISABLE 0x0003

typedef enum {
  ANALOG_AIN0 = 0,
  ANALOG_AIN1 = 1,
  ANALOG_AIN2 = 2,
  ANALOG_AIN3 = 3
} AnalogAinChannel_t;

/**
 * @brief Initialize ADS1115 analog manager.
 *
 * Port mapping:
 * - PORT_1 -> AIN1
 * - PORT_2 -> AIN2
 * - PORT_3 -> AIN3
 *
 * @return MRS_OK on success.
 */
system_err_t AnalogManager_Init(void);

/**
 * @brief Deinitialize ADS1115 analog manager.
 */
system_err_t AnalogManager_Deinit(void);

/**
 * @brief Select active logical sensor port used by AnalogManager_ReadCurrentPort().
 */
system_err_t AnalogManager_SetCurrentPort(PortId_t port);

/**
 * @brief Read the active logical port (PORT_1/2/3) mapped channel.
 */
system_err_t AnalogManager_ReadCurrentPort(SensorData_t *data);

/**
 * @brief Read battery/capacity analog pin from ADS1115 AIN3.
 */
system_err_t AnalogManager_ReadCapacityPin(SensorData_t *data);

/**
 * @brief Read one explicit ADS1115 AIN channel.
 */
system_err_t AnalogManager_ReadAin(AnalogAinChannel_t ain, SensorData_t *data);

#ifdef __cplusplus
}
#endif

#endif /* ANALOG_MANAGER_H */
