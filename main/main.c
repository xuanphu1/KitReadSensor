#include "main.h"
#include "Common.h"
#include "LedRGB.h"
#include "nvs_flash.h"

void app_main(void) {

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  
  // Khởi tạo LED RGB
  esp_err_t led_ret = LedRGB_Init();
  if (led_ret != ESP_OK) {
    ESP_LOGW(TAG_MAIN, "Failed to initialize LED RGB: %s", esp_err_to_name(led_ret));
  }
  
  ButtonManagerInit();
  ESP_ERROR_CHECK(i2cInitDevCommon());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  MainScreen = ssd1306_create(I2C_NUM_0, SSD1306_I2C_ADDRESS);
#pragma GCC diagnostic pop
  if (MainScreen == NULL) {
    ESP_LOGE(TAG_MAIN, "Failed to create SSD1306 handle");
    vTaskDelay(portMAX_DELAY);
  }
  ScreenManagerInit(&MainScreen);
  MenuSystemInit(&DataManager);
  xTaskCreate(wifi_init_sta, "wifi_init_sta", 4096, &DataManager, 5, NULL);
  xTaskCreate(NavigationScreen_Task, "NavigationScreen_Task", 4096,
              &DataManager, 5, NULL);
  while (1) {
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

