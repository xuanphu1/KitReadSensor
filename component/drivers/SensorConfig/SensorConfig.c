#include "SensorConfig.h"

bmp280_t bme280_device;
bmp280_params_t bme280_params;

/* -------------------- BME280 Driver Wrapper Functions -------------------- */

void sensor_bme280_init(void) {
  esp_err_t ret = bme280_init(&bme280_device, &bme280_params, BME280_ADDRESS,
                               CONFIG_I2CDEV_COMMON_PORT, CONFIG_I2CDEV_COMMON_SDA,
                               CONFIG_I2CDEV_COMMON_SCL);
  if (ret != ESP_OK) {
    ESP_LOGE("sensor_bme280_init", "bme280_init failed: %s", esp_err_to_name(ret));
    return;
  }
}

void sensor_bme280_read(SensorData_t *data) {
  esp_err_t ret = bme280_readSensorData(&bme280_device,
                                        &data->data_fl[0],
                                        &data->data_fl[1],
                                        &data->data_fl[2]);
  if (ret != ESP_OK) {
    ESP_LOGE("sensor_bme280_read", "read failed: %s", esp_err_to_name(ret));
    return;
  }
}

void sensor_bme280_deinit(void) {
  // TODO: Implement deinit if needed
  // ESP_ERROR_CHECK(bme280_deinit(&bme280_device));
}

/* -------------------- Sensor Config Functions -------------------- */

void SensorConfigInit(void) {
  // Khởi tạo các sensor đã được cấu hình
  // Có thể mở rộng để init nhiều sensor khác
}

void SensorConfigRead(SensorData_t *data) {
  // Đọc dữ liệu từ các sensor đã được cấu hình
  // Có thể mở rộng để đọc từ nhiều sensor khác
}

void SensorConfigDeinit(void) {
  // Giải phóng tài nguyên của các sensor
  // Có thể mở rộng để deinit nhiều sensor khác
}

