#include "AnalogManager.h"
#include <string.h>
#include "PinManager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2cdev.h"

typedef struct {
  bool initialized;
  PortId_t current_port;
  i2c_dev_t dev;
  AnalogAinChannel_t port_to_ain[NUM_PORTS];
} analog_manager_ctx_t;

static analog_manager_ctx_t s_ctx = {
    .initialized = false,
    .current_port = PORT_1,
    .port_to_ain = {ANALOG_AIN1, ANALOG_AIN2, ANALOG_AIN3},
};

static system_err_t validate_port(PortId_t port) {
  if (port < PORT_1 || port >= NUM_PORTS) {
    ESP_LOGE(TAG_ANALOG_MANAGER, "Invalid port: %d", (int)port);
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  return MRS_OK;
}

static system_err_t read_channel_internal(AnalogAinChannel_t ain, SensorData_t *data) {
  if (data == NULL) {
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  if (!s_ctx.initialized) {
    return MRS_ERR_CORE_NOT_INITIALIZED;
  }
  if (ain > ANALOG_AIN3) {
    return MRS_ERR_CORE_INVALID_PARAM;
  }

  uint16_t mux = ADS_CFG_MUX_AIN0;
  switch (ain) {
  case ANALOG_AIN0:
    mux = ADS_CFG_MUX_AIN0;
    break;
  case ANALOG_AIN1:
    mux = ADS_CFG_MUX_AIN1;
    break;
  case ANALOG_AIN2:
    mux = ADS_CFG_MUX_AIN2;
    break;
  case ANALOG_AIN3:
    mux = ADS_CFG_MUX_AIN3;
    break;
  default:
    return MRS_ERR_CORE_INVALID_PARAM;
  }

  uint16_t cfg = (uint16_t)(ADS_CFG_OS_START | mux | ADS_CFG_PGA_4V096 |
                            ADS_CFG_MODE_SINGLE | ADS_CFG_DR_128SPS |
                            ADS_CFG_COMP_DISABLE);
  uint8_t cfg_buf[2] = {(uint8_t)(cfg >> 8), (uint8_t)(cfg & 0xFF)};
  esp_err_t ret = i2c_dev_write_reg(&s_ctx.dev, ADS_REG_CONFIG, cfg_buf, sizeof(cfg_buf));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_ANALOG_MANAGER, "Write cfg failed: %s", esp_err_to_name(ret));
    return MRS_ERR_DRIVERS_COMM_FAILED;
  }

  vTaskDelay(pdMS_TO_TICKS(10));

  uint8_t raw_buf[2] = {0};
  ret = i2c_dev_read_reg(&s_ctx.dev, ADS_REG_CONVERSION, raw_buf, sizeof(raw_buf));
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_ANALOG_MANAGER, "Read conversion failed: %s", esp_err_to_name(ret));
    return MRS_ERR_DRIVERS_COMM_FAILED;
  }

  uint16_t raw_u16 = (uint16_t)((raw_buf[0] << 8) | raw_buf[1]);
  int16_t raw_i16 = (int16_t)raw_u16;
  float voltage = ((float)raw_i16 * 4.096f) / 32768.0f;

  data->data_fl[0] = (float)raw_u16;
  data->data_fl[1] = voltage;
  data->data_uint16[0] = raw_u16;
  data->data_uint32[0] = (uint32_t)raw_u16;
  data->data_uint8[0] = (uint8_t)(raw_u16 & 0xFF);
  return MRS_OK;
}

system_err_t AnalogManager_Init(void) {
  if (s_ctx.initialized) {
    return MRS_OK;
  }

  memset(&s_ctx.dev, 0, sizeof(s_ctx.dev));
  esp_err_t ret = i2c_dev_create_mutex(&s_ctx.dev);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_ANALOG_MANAGER, "i2c_dev_create_mutex failed: %s", esp_err_to_name(ret));
    return MRS_ERR_DRIVERS_INIT_FAILED;
  }
  s_ctx.dev.port = PIN_I2C_PORT_NUM;
  s_ctx.dev.addr = ANALOG_ADS_ADDR;
  s_ctx.dev.cfg.sda_io_num = PIN_I2C_SDA;
  s_ctx.dev.cfg.scl_io_num = PIN_I2C_SCL;
#if HELPER_TARGET_IS_ESP32
  s_ctx.dev.cfg.master.clk_speed = PIN_I2C_CLK_HZ;
  s_ctx.dev.cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
  s_ctx.dev.cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
  s_ctx.dev.cfg.clk_flags = 0;
  s_ctx.dev.timeout_ticks = I2CDEV_MAX_STRETCH_TIME;
#endif

  ret = i2c_dev_probe(&s_ctx.dev, I2C_DEV_WRITE);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_ANALOG_MANAGER, "ADS1115 probe failed at 0x%02X: %s",
             ANALOG_ADS_ADDR, esp_err_to_name(ret));
    return MRS_ERR_DRIVERS_COMM_FAILED;
  }

  s_ctx.port_to_ain[PORT_1] = ANALOG_AIN1;
  s_ctx.port_to_ain[PORT_2] = ANALOG_AIN2;
  s_ctx.port_to_ain[PORT_3] = ANALOG_AIN3;
  s_ctx.current_port = PORT_1;
  s_ctx.initialized = true;

  ESP_LOGI(TAG_ANALOG_MANAGER, "ADS1115 init done: P1->AIN1, P2->AIN2, P3->AIN3, CAP->AIN3");
  return MRS_OK;
}

system_err_t AnalogManager_Deinit(void) {
  if (!s_ctx.initialized) {
    return MRS_OK;
  }

  i2c_dev_delete_mutex(&s_ctx.dev);

  s_ctx.initialized = false;
  return MRS_OK;
}

system_err_t AnalogManager_SetCurrentPort(PortId_t port) {
  system_err_t v = validate_port(port);
  if (v != MRS_OK) {
    return v;
  }
  s_ctx.current_port = port;
  return MRS_OK;
}

system_err_t AnalogManager_ReadCurrentPort(SensorData_t *data) {
  system_err_t v = validate_port(s_ctx.current_port);
  if (v != MRS_OK) {
    return v;
  }
  AnalogAinChannel_t ain = s_ctx.port_to_ain[s_ctx.current_port];
  return read_channel_internal(ain, data);
}

system_err_t AnalogManager_ReadCapacityPin(SensorData_t *data) {
  return read_channel_internal(ANALOG_AIN3, data);
}

system_err_t AnalogManager_ReadAin(AnalogAinChannel_t ain, SensorData_t *data) {
  return read_channel_internal(ain, data);
}
