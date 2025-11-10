#include "SensorSystem.h"

bmp280_t bme280_device;
bmp280_params_t bme280_params;

void bme280Init(void) {
    esp_err_t ret = bme280_init(&bme280_device, &bme280_params, BME280_ADDRESS,
                      CONFIG_I2CDEV_COMMON_PORT, CONFIG_I2CDEV_COMMON_SDA,
                      CONFIG_I2CDEV_COMMON_SCL);
    if (ret != ESP_OK) {
      ESP_LOGE("bme280_init", "bme280_init failed: %s", esp_err_to_name(ret));
      return;
    }
  }

  void bme280Read(SensorData_t *data) {
    esp_err_t ret = bme280_readSensorData(&bme280_device,
                                          &data->data_fl[0],
                                          &data->data_fl[1],
                                          &data->data_fl[2]);
    if (ret != ESP_OK) {
      ESP_LOGE("bme280_readSensorData", "read failed: %s", esp_err_to_name(ret));
      return;
    }
  }

void bme280Deinit(void) {
//   ESP_ERROR_CHECK(bme280_deinit(&bme280_device));
}
