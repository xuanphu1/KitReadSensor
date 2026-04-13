#include "PowerManager.h"
#include <stdio.h>
#include <string.h>
#include "AnalogManager.h"
#include "PinManager.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "pcf8574.h"

typedef struct {
  bool initialized;
  i2c_dev_t pcf_dev;
  uint8_t cached_port;
  SemaphoreHandle_t lock;
  char status_text[32];
} PowerManagerContext_t;

static PowerManagerContext_t s_power = {
    .initialized = false,
    .cached_port = 0xFF,
    .lock = NULL,
    .status_text = "PWR N/A",
};

static inline uint8_t rail_to_pin(PowerRail_t rail) {
  switch (rail) {
  case POWER_RAIL_3V3_PERIPHERAL:
    return POWER_PIN_3V3_PERIPHERAL;
  case POWER_RAIL_5V_PERIPHERAL:
    return POWER_PIN_5V_PERIPHERAL;
  case POWER_RAIL_3V3_LORA:
    return POWER_PIN_3V3_LORA;
  default:
    return 0xFF;
  }
}

static inline bool rail_active_high(PowerRail_t rail) {
  switch (rail) {
  case POWER_RAIL_3V3_PERIPHERAL:
    return false; // ON at LOW level
  case POWER_RAIL_5V_PERIPHERAL:
    return true; // ON at HIGH level
  case POWER_RAIL_3V3_LORA:
    return false; // ON at LOW level
  default:
    return true;
  }
}

static inline bool decode_pin_enabled(uint8_t port_value, uint8_t pin) {
  PowerRail_t rail = POWER_RAIL_MAX;
  if (pin == POWER_PIN_3V3_PERIPHERAL) {
    rail = POWER_RAIL_3V3_PERIPHERAL;
  } else if (pin == POWER_PIN_5V_PERIPHERAL) {
    rail = POWER_RAIL_5V_PERIPHERAL;
  } else if (pin == POWER_PIN_3V3_LORA) {
    rail = POWER_RAIL_3V3_LORA;
  }

  bool raw_high = ((port_value >> pin) & 0x01U) != 0U;
  if (rail == POWER_RAIL_MAX) {
    return raw_high;
  }
  return rail_active_high(rail) ? raw_high : !raw_high;
}

static inline void apply_pin_enable(uint8_t *port_value, uint8_t pin, bool enable) {
  PowerRail_t rail = POWER_RAIL_MAX;
  if (pin == POWER_PIN_3V3_PERIPHERAL) {
    rail = POWER_RAIL_3V3_PERIPHERAL;
  } else if (pin == POWER_PIN_5V_PERIPHERAL) {
    rail = POWER_RAIL_5V_PERIPHERAL;
  } else if (pin == POWER_PIN_3V3_LORA) {
    rail = POWER_RAIL_3V3_LORA;
  }

  bool active_high = true;
  if (rail != POWER_RAIL_MAX) {
    active_high = rail_active_high(rail);
  }

  if (active_high) {
    if (enable) {
      *port_value |= (1U << pin);
    } else {
      *port_value &= (uint8_t) ~(1U << pin);
    }
  } else {
    if (enable) {
      *port_value &= (uint8_t) ~(1U << pin);
    } else {
      *port_value |= (1U << pin);
    }
  }
}

static esp_err_t write_cached_port_locked(void) {
  esp_err_t ret = pcf8574_port_write(&s_power.pcf_dev, s_power.cached_port);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_POWER_MANAGER, "PCF8574 write failed: %s", esp_err_to_name(ret));
  }
  return ret;
}

static esp_err_t read_port_locked(uint8_t *value_out) {
  esp_err_t ret = pcf8574_port_read(&s_power.pcf_dev, value_out);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_POWER_MANAGER, "PCF8574 read failed: %s", esp_err_to_name(ret));
  }
  return ret;
}

static uint8_t build_mask_from_port(uint8_t port_value) {
  uint8_t mask = 0;
  if (decode_pin_enabled(port_value, POWER_PIN_3V3_PERIPHERAL)) {
    mask |= (1U << POWER_RAIL_3V3_PERIPHERAL);
  }
  if (decode_pin_enabled(port_value, POWER_PIN_5V_PERIPHERAL)) {
    mask |= (1U << POWER_RAIL_5V_PERIPHERAL);
  }
  if (decode_pin_enabled(port_value, POWER_PIN_3V3_LORA)) {
    mask |= (1U << POWER_RAIL_3V3_LORA);
  }
  return mask;
}

static void update_status_text_from_mask(uint8_t mask) {
  uint8_t on_count = 0;
  if (mask & (1U << POWER_RAIL_3V3_PERIPHERAL)) {
    on_count++;
  }
  if (mask & (1U << POWER_RAIL_5V_PERIPHERAL)) {
    on_count++;
  }
  if (mask & (1U << POWER_RAIL_3V3_LORA)) {
    on_count++;
  }
  snprintf(s_power.status_text, sizeof(s_power.status_text), "PWR %u/3 ON", on_count);
}

esp_err_t PowerManager_Init(void) {
  if (s_power.initialized) {
    return ESP_OK;
  }

  if (s_power.lock == NULL) {
    s_power.lock = xSemaphoreCreateMutex();
    if (s_power.lock == NULL) {
      return ESP_ERR_NO_MEM;
    }
  }

  memset(&s_power.pcf_dev, 0, sizeof(s_power.pcf_dev));

  esp_err_t ret = pcf8574_init_desc(&s_power.pcf_dev, POWER_PCF8574_ADDR, PIN_I2C_PORT_NUM,
                                    PIN_I2C_SDA, PIN_I2C_SCL);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_POWER_MANAGER, "pcf8574_init_desc failed: %s", esp_err_to_name(ret));
    return ret;
  }

  if (xSemaphoreTake(s_power.lock, portMAX_DELAY) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }

  ret = read_port_locked(&s_power.cached_port);
  if (ret != ESP_OK) {
    xSemaphoreGive(s_power.lock);
    return ret;
  }

  apply_pin_enable(&s_power.cached_port, POWER_PIN_3V3_PERIPHERAL,
                   POWER_DEFAULT_3V3_PERIPHERAL_ON != 0);
  apply_pin_enable(&s_power.cached_port, POWER_PIN_5V_PERIPHERAL, POWER_DEFAULT_5V_PERIPHERAL_ON != 0);
  apply_pin_enable(&s_power.cached_port, POWER_PIN_3V3_LORA, POWER_DEFAULT_3V3_LORA_ON != 0);

  ret = write_cached_port_locked();
  if (ret == ESP_OK) {
    uint8_t mask = build_mask_from_port(s_power.cached_port);
    update_status_text_from_mask(mask);
    s_power.initialized = true;
    ESP_LOGI(TAG_POWER_MANAGER, "Initialized, mask=0x%02X", mask);
  }

  xSemaphoreGive(s_power.lock);
  return ret;
}

esp_err_t PowerManager_Deinit(void) {
  if (!s_power.initialized) {
    return ESP_OK;
  }

  esp_err_t ret = pcf8574_free_desc(&s_power.pcf_dev);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_POWER_MANAGER, "pcf8574_free_desc failed: %s", esp_err_to_name(ret));
    return ret;
  }

  s_power.initialized = false;
  return ESP_OK;
}

esp_err_t PowerManager_SetRail(PowerRail_t rail, bool enable) {
  if (!s_power.initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t pin = rail_to_pin(rail);
  if (pin == 0xFF) {
    return ESP_ERR_INVALID_ARG;
  }

  if (xSemaphoreTake(s_power.lock, portMAX_DELAY) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }

  apply_pin_enable(&s_power.cached_port, pin, enable);
  esp_err_t ret = write_cached_port_locked();
  if (ret == ESP_OK) {
    uint8_t mask = build_mask_from_port(s_power.cached_port);
    update_status_text_from_mask(mask);
  }

  xSemaphoreGive(s_power.lock);
  return ret;
}

esp_err_t PowerManager_SetMask(uint8_t mask) {
  if (!s_power.initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  if (xSemaphoreTake(s_power.lock, portMAX_DELAY) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }

  apply_pin_enable(&s_power.cached_port, POWER_PIN_3V3_PERIPHERAL,
                   (mask & (1U << POWER_RAIL_3V3_PERIPHERAL)) != 0U);
  apply_pin_enable(&s_power.cached_port, POWER_PIN_5V_PERIPHERAL,
                   (mask & (1U << POWER_RAIL_5V_PERIPHERAL)) != 0U);
  apply_pin_enable(&s_power.cached_port, POWER_PIN_3V3_LORA, (mask & (1U << POWER_RAIL_3V3_LORA)) != 0U);

  esp_err_t ret = write_cached_port_locked();
  if (ret == ESP_OK) {
    update_status_text_from_mask(mask);
  }

  xSemaphoreGive(s_power.lock);
  return ret;
}

esp_err_t PowerManager_GetMask(uint8_t *mask_out) {
  if (mask_out == NULL) {
    return ESP_ERR_INVALID_ARG;
  }
  if (!s_power.initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  if (xSemaphoreTake(s_power.lock, portMAX_DELAY) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }

  uint8_t port_value = 0;
  esp_err_t ret = read_port_locked(&port_value);
  if (ret == ESP_OK) {
    s_power.cached_port = port_value;
    *mask_out = build_mask_from_port(port_value);
    update_status_text_from_mask(*mask_out);
  }

  xSemaphoreGive(s_power.lock);
  return ret;
}

esp_err_t PowerManager_GetState(PowerRailState_t *state_out) {
  if (state_out == NULL) {
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t mask = 0;
  esp_err_t ret = PowerManager_GetMask(&mask);
  if (ret != ESP_OK) {
    return ret;
  }

  state_out->rail_3v3_peripheral = (mask & (1U << POWER_RAIL_3V3_PERIPHERAL)) != 0U;
  state_out->rail_5v_peripheral = (mask & (1U << POWER_RAIL_5V_PERIPHERAL)) != 0U;
  state_out->rail_3v3_lora = (mask & (1U << POWER_RAIL_3V3_LORA)) != 0U;
  return ESP_OK;
}

void PowerManager_UpdateInfo(BatteryInfo_t *batteryInfo) {
  if (batteryInfo == NULL) {
    return;
  }

  SensorData_t analog_data = {0};
  if (AnalogManager_Init() != MRS_OK ||
      AnalogManager_ReadAin(ANALOG_AIN0, &analog_data) != MRS_OK) {
    batteryInfo->batteryLevel = 0;
    batteryInfo->batteryName = "AIN0 ERR";
    return;
  }

  /* ADS1115 single-ended raw ~= 0..32767. Show percentage directly from divider output. */
  uint32_t raw = analog_data.data_uint16[0];
  if (raw > 32767U) {
    raw = 32767U;
  }
  uint8_t percent = (uint8_t)((raw * 100U) / 32767U);
  if (percent > 100U) {
    percent = 100U;
  }

  if (percent == 0U) {
    batteryInfo->batteryLevel = 0U;
  } else if (percent <= 17U) {
    batteryInfo->batteryLevel = 1U;
  } else if (percent <= 33U) {
    batteryInfo->batteryLevel = 2U;
  } else if (percent <= 50U) {
    batteryInfo->batteryLevel = 3U;
  } else if (percent <= 67U) {
    batteryInfo->batteryLevel = 4U;
  } else if (percent <= 83U) {
    batteryInfo->batteryLevel = 5U;
  } else {
    batteryInfo->batteryLevel = 6U;
  }

  snprintf(s_power.status_text, sizeof(s_power.status_text), "%u%%", percent);
  batteryInfo->batteryName = s_power.status_text;
}
