#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include "DataManager.h"
#include "esp_err.h"

#define TAG_POWER_MANAGER "POWER_MANAGER"

#define POWER_PCF8574_ADDR 0x20

#define POWER_PIN_3V3_PERIPHERAL 4
#define POWER_PIN_5V_PERIPHERAL 6
#define POWER_PIN_3V3_LORA 5

/* Default startup state */
#define POWER_DEFAULT_3V3_PERIPHERAL_ON 0
#define POWER_DEFAULT_5V_PERIPHERAL_ON 0
#define POWER_DEFAULT_3V3_LORA_ON 0

/**
 * @file PowerManager.h
 * @brief Power rail control via PCF8574 for external MOSFET switches.
 */

typedef enum {
  POWER_RAIL_3V3_PERIPHERAL = 0,
  POWER_RAIL_5V_PERIPHERAL,
  POWER_RAIL_3V3_LORA,
  POWER_RAIL_MAX
} PowerRail_t;

typedef struct {
  bool rail_3v3_peripheral;
  bool rail_5v_peripheral;
  bool rail_3v3_lora;
} PowerRailState_t;

/**
 * @brief Initialize PowerManager and bind to PCF8574.
 *
 * Must be called once before any set/get power operation.
 *
 * @return ESP_OK on success.
 */
esp_err_t PowerManager_Init(void);

/**
 * @brief Release PowerManager resources.
 *
 * @return ESP_OK on success.
 */
esp_err_t PowerManager_Deinit(void);

/**
 * @brief Set one rail ON/OFF.
 *
 * @param rail Selected rail.
 * @param enable true = ON, false = OFF.
 * @return ESP_OK on success.
 */
esp_err_t PowerManager_SetRail(PowerRail_t rail, bool enable);

/**
 * @brief Set all managed rails by bitmask.
 *
 * Mask bits use PowerRail_t index: bit0=3v3 peripheral, bit1=5v peripheral,
 * bit2=3v3 LoRa.
 *
 * @param mask Enabled rails mask.
 * @return ESP_OK on success.
 */
esp_err_t PowerManager_SetMask(uint8_t mask);

/**
 * @brief Get current software-cached rail mask.
 *
 * @param mask_out Output pointer for mask.
 * @return ESP_OK on success.
 */
esp_err_t PowerManager_GetMask(uint8_t *mask_out);

/**
 * @brief Read and decode current states from hardware.
 *
 * @param state_out Output pointer for decoded rail states.
 * @return ESP_OK on success.
 */
esp_err_t PowerManager_GetState(PowerRailState_t *state_out);

/**
 * @brief Optional helper for current UI flow.
 *
 * Reuses BatteryInfo_t fields to show power status text and icon level.
 *
 * @param batteryInfo Destination object; must not be NULL.
 */
void PowerManager_UpdateInfo(BatteryInfo_t *batteryInfo);

#endif /* POWER_MANAGER_H */
