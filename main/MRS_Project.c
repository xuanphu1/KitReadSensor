#include "MRS_Project.h"
#include "DataManager.h"
#include "nvs_flash.h"

bmp280_t bme280_device;
bmp280_params_t bme280_params;

void ReadBME280Data(void *pvParameter) {
  while (1) {
    float temp, press, humidity;
    ESP_ERROR_CHECK(
        bme280_readSensorData(&bme280_device, &temp, &press, &humidity));
      ESP_LOGI(TAG_MAIN, "Temperature: %f, Pressure: %f, Humidity: %f", temp,
               press, humidity);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
void app_main(void) {

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  MenuButtonInit();
  ESP_ERROR_CHECK(i2cdev_init());
  ESP_ERROR_CHECK(bme280_init(&bme280_device, &bme280_params, BME280_ADDRESS,
                              CONFIG_BME_I2C_PORT, CONFIG_BME_PIN_NUM_SDA,
                              CONFIG_BME_PIN_NUM_SCL));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  MainScreen = ssd1306_create(I2C_NUM_0, SSD1306_I2C_ADDRESS);
#pragma GCC diagnostic pop
  if (MainScreen == NULL) {
    ESP_LOGE(TAG_MAIN, "Failed to create SSD1306 handle");
    vTaskDelay(portMAX_DELAY);
  }
  ScreenManagerInit(&MainScreen);
  MenuSystemInit();
  xTaskCreate(wifi_init_sta, "wifi_init_sta", 4096, &DataManager, 5, NULL);
  xTaskCreate(NavigationScreen_Task, "NavigationScreen_Task", 4096,
              &DataManager, 5, NULL);
  xTaskCreate(ReadBME280Data, "ReadBME280Data", 4096, NULL, 5, NULL);
  while (1) {
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
