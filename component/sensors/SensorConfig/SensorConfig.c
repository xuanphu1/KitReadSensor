#include "SensorConfig.h"
#include "driver/adc.h"
#include "aht.h"

bmp280_t bme280_device;
bmp280_params_t bme280_params;
static bool bme280_initialized = false;

static aht_t aht10_device;
static bool aht10_initialized = false;

/* -------------------- MQ analog state -------------------- */
static PortId_t s_current_port = PORT_1;
static bool mq_adc_initialized = false;
static adc1_channel_t mq_adc_channels[NUM_PORTS];
static int mq_analog_gpios[NUM_PORTS]; /* GPIO tương ứng mỗi port để log */

/* -------------------- BME280 Driver Wrapper Functions -------------------- */

system_err_t bme280Initialize(void) {
  if (bme280_initialized) {
    ESP_LOGW("sensor_bme280_init", "BME280 already initialized");
    return MRS_OK;
  }
  
  esp_err_t ret = bme280_init(&bme280_device, &bme280_params, BME280_ADDRESS,
                               CONFIG_I2CDEV_COMMON_PORT, CONFIG_I2CDEV_COMMON_SDA,
                               CONFIG_I2CDEV_COMMON_SCL);
  if (ret != ESP_OK) {
    ESP_LOGE("sensor_bme280_init", "bme280_init failed: %s", esp_err_to_name(ret));
    return MRS_ERR_SENSOR_BME280_INIT_FAILED;
  }
  
  bme280_initialized = true;
  return MRS_OK;
}

system_err_t bme280ReadData(SensorData_t *data) {
  if (data == NULL) {
    ESP_LOGE("sensor_bme280_read", "data pointer is NULL");
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  
  if (!bme280_initialized) {
    ESP_LOGE("sensor_bme280_read", "BME280 not initialized");
    return MRS_ERR_SENSORS_NOT_INITIALIZED;
  }
  
  esp_err_t ret = bme280_readSensorData(&bme280_device,
                                        &data->data_fl[0],
                                        &data->data_fl[1],
                                        &data->data_fl[2]);
  if (ret != ESP_OK) {
    ESP_LOGE("sensor_bme280_read", "read failed: %s", esp_err_to_name(ret));
    return MRS_ERR_SENSOR_BME280_READ_FAILED;
  }
  
  return MRS_OK;
}

system_err_t bme280Deinitialize(void) {
  if (!bme280_initialized) {
    return MRS_OK;
  }
  
  // TODO: Implement deinit if needed
  // ESP_ERROR_CHECK(bme280_deinit(&bme280_device));
  bme280_initialized = false;
  return MRS_OK;
}

/* -------------------- AHT10 Driver Wrapper Functions -------------------- */

system_err_t aht10Initialize(void) {
  if (aht10_initialized) {
    ESP_LOGW("sensor_aht10_init", "AHT10 already initialized");
    return MRS_OK;
  }

  esp_err_t ret =
      aht_init_desc(&aht10_device, AHT_I2C_ADDRESS_GND,
                    CONFIG_I2CDEV_COMMON_PORT, CONFIG_I2CDEV_COMMON_SDA,
                    CONFIG_I2CDEV_COMMON_SCL);
  if (ret != ESP_OK) {
    ESP_LOGE("sensor_aht10_init", "aht_init_desc failed: %s",
             esp_err_to_name(ret));
    return MRS_ERR_I2CDEV_INIT_FAILED;
  }

  aht10_device.type = AHT_TYPE_AHT1x;
  aht10_device.mode = AHT_MODE_NORMAL;

  ret = aht_init(&aht10_device);
  if (ret != ESP_OK) {
    ESP_LOGE("sensor_aht10_init", "aht_init failed: %s",
             esp_err_to_name(ret));
    return MRS_ERR_DRIVERS_INIT_FAILED;
  }

  aht10_initialized = true;
  return MRS_OK;
}

system_err_t aht10ReadData(SensorData_t *data) {
  if (data == NULL) {
    ESP_LOGE("sensor_aht10_read", "data pointer is NULL");
    return MRS_ERR_CORE_INVALID_PARAM;
  }

  if (!aht10_initialized) {
    system_err_t ret = aht10Initialize();
    if (ret != MRS_OK) {
      return ret;
    }
  }

  float temp = 0.0f, hum = 0.0f;
  esp_err_t ret = aht_get_data(&aht10_device, &temp, &hum);
  if (ret != ESP_OK) {
    ESP_LOGE("sensor_aht10_read", "aht_get_data failed: %s",
             esp_err_to_name(ret));
    return MRS_ERR_DRIVERS_COMM_FAILED;
  }

  data->data_fl[0] = temp;
  data->data_fl[1] = hum;
  return MRS_OK;
}

system_err_t aht10Deinitialize(void) {
  if (!aht10_initialized) {
    return MRS_OK;
  }

  esp_err_t ret = aht_free_desc(&aht10_device);
  if (ret != ESP_OK) {
    ESP_LOGW("sensor_aht10_deinit", "aht_free_desc failed: %s",
             esp_err_to_name(ret));
  }
  aht10_initialized = false;
  return MRS_OK;
}

/* -------------------- Sensor Config Functions -------------------- */

system_err_t SensorConfigInit(void) {
  // Khởi tạo các sensor đã được cấu hình
  // Có thể mở rộng để init nhiều sensor khác
  return MRS_OK;
}

system_err_t SensorConfigRead(SensorData_t *data) {
  if (data == NULL) {
    ESP_LOGE("SensorConfig", "SensorConfigRead: data is NULL");
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  
  // Đọc dữ liệu từ các sensor đã được cấu hình
  // Có thể mở rộng để đọc từ nhiều sensor khác
  return MRS_OK;
}

system_err_t SensorConfigDeinit(void) {
  // Giải phóng tài nguyên của các sensor
  // Có thể mở rộng để deinit nhiều sensor khác
  return bme280Deinitialize();
}

/* -------------------- MQ Analog Sensors (MQ2, MQ3, ...) -------------------- */
// Đặt port hiện tại (được gọi từ FunctionManager trước khi đọc)
void SensorConfigSetCurrentPort(PortId_t port) {
  if (port < PORT_1 || port >= NUM_PORTS) {
    return;
  }
  s_current_port = port;
}

system_err_t mq_analog_init(void) {
#if CONFIG_IDF_TARGET_ESP32
  if (mq_adc_initialized) {
    ESP_LOGI("sensor_mq_init", "MQ analog ADC already initialized");
    return MRS_OK;
  }

  esp_err_t ret = adc1_config_width(ADC_WIDTH_BIT_12);
  if (ret != ESP_OK) {
    ESP_LOGE("sensor_mq_init", "adc1_config_width failed: %s",
             esp_err_to_name(ret));
    return MRS_ERR_DRIVERS_INIT_FAILED; // tái sử dụng mã lỗi ADC
  }

  const int analog_gpios[NUM_PORTS] = {
      CONFIG_IO_1_PORT_1, // PORT_1
      CONFIG_IO_3_PORT_2, // PORT_2
      CONFIG_IO_1_PORT_3  // PORT_3
  };

  for (int i = 0; i < NUM_PORTS; i++) {
    int gpio = analog_gpios[i];
    adc1_channel_t ch;
    switch (gpio) {
    case 32:
      ch = ADC1_CHANNEL_4;
      break;
    case 33:
      ch = ADC1_CHANNEL_5;
      break;
    case 34:
      ch = ADC1_CHANNEL_6;
      break;
    case 35:
      ch = ADC1_CHANNEL_7;
      break;
    case 36:
      ch = ADC1_CHANNEL_0;
      break;
    case 39:
      ch = ADC1_CHANNEL_3;
      break;
    default:
      ESP_LOGW("sensor_mq_init",
               "GPIO %d is not ADC1 channel on ESP32, skip mapping for port %d",
               gpio, i);
      continue;
    }

    ret = adc1_config_channel_atten(ch, ADC_ATTEN_DB_11);
    if (ret != ESP_OK) {
      ESP_LOGE("sensor_mq_init",
               "adc1_config_channel_atten failed for GPIO %d: %s", gpio,
               esp_err_to_name(ret));
      continue;
    }
    mq_adc_channels[i] = ch;
    mq_analog_gpios[i] = gpio;
    ESP_LOGI("sensor_mq_init", "Port %d analog GPIO %d mapped to ADC1 channel %d",
             i + 1, gpio, ch);
  }

  mq_adc_initialized = true;
  return MRS_OK;
#else
  // Trên target khác (ví dụ ESP32-C6) chưa hiện thực đọc ADC cho MQ
  ESP_LOGW("sensor_mq_init",
           "MQ analog init: ADC mapping not implemented for this target");
  return MRS_OK;
#endif
}

system_err_t mq_analog_read(SensorData_t *data) {
  if (data == NULL) {
    ESP_LOGE("sensor_mq_read", "data pointer is NULL");
    return MRS_ERR_CORE_INVALID_PARAM;
  }

#if CONFIG_IDF_TARGET_ESP32
  if (!mq_adc_initialized) {
    system_err_t ret = mq_analog_init();
    if (ret != MRS_OK) {
      return ret;
    }
  }

  int port = (int)s_current_port;
  if (port < PORT_1 || port >= NUM_PORTS) {
    ESP_LOGW("sensor_mq_read", "Invalid current port: %d", port);
    return MRS_ERR_CORE_INVALID_PARAM;
  }

  adc1_channel_t ch = mq_adc_channels[port];
  int gpio = mq_analog_gpios[port];
  int raw = adc1_get_raw(ch);
  if (raw < 0) {
    ESP_LOGW("sensor_mq_read", "adc1_get_raw failed for port %d (ch=%d): %d",
             port + 1, ch, raw);
    return MRS_ERR_SENSORS_NOT_INITIALIZED;
  }

  ESP_LOGI("sensor_mq_read", "Port %d: dang doc chan GPIO %d, ADC raw = %d",
           port + 1, gpio, raw);

  data->data_fl[0] = (float)raw;
  data->data_uint16[0] = (uint16_t)raw;
  data->data_uint32[0] = (uint32_t)raw;
  data->data_uint8[0] = (uint8_t)(raw & 0xFF);

  return MRS_OK;
#else
  // Target khác: tạm thời trả giá trị 0 thực nhưng không có ADC
  data->data_fl[0] = 0.0f;
  data->data_uint16[0] = 0;
  data->data_uint32[0] = 0;
  data->data_uint8[0] = 0;
  return MRS_OK;
#endif
}

