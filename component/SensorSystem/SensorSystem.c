#include "SensorSystem.h"

bmp280_t bme280_device;
bmp280_params_t bme280_params;

void bme280Init(void) {
  ESP_ERROR_CHECK(i2cdev_init());
  ESP_ERROR_CHECK(bme280_init(&bme280_device, &bme280_params, BME280_ADDRESS,
                              CONFIG_BME_I2C_PORT, CONFIG_BME_PIN_NUM_SDA,
                              CONFIG_BME_PIN_NUM_SCL));
}

void bme280Read(SensorData_t *data) {
  ESP_ERROR_CHECK(bme280_readSensorData(&bme280_device, &data->data_fl[0], &data->data_fl[1], &data->data_fl[2]));
}

void bme280Deinit(void) {
//   ESP_ERROR_CHECK(bme280_deinit(&bme280_device));
}
