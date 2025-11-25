#include "SensorConfig.h"

bmp280_t bme280_device;
bmp280_params_t bme280_params;
static bool bme280_initialized = false;

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

/* -------------------- Wrapper Functions for SensorRegistry -------------------- */
// Các hàm wrapper với tên tương thích với SensorRegistry

