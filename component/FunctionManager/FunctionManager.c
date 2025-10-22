#include "FunctionManager.h"
#include "ScreenManager.h"

void wifi_config_task(void *pvParameters) {
  DataManager_t *data = (DataManager_t *)pvParameters;
  wifi_init_ap();

  if (data->objectInfo.wifiInfo.wifiStatus == CONNECTED)
    data->objectInfo.wifiInfo.wifiStatus = DISCONNECTED;
  while (1) {
    update_wifi_status(&(data->objectInfo.wifiInfo));
    wifi_connect_handler(data);
    ScreenWifiCallback(data);
    if (data->objectInfo.wifiInfo.wifiStatus == CONNECTED) {
      MenuRender(data->MenuReturn[0], &(data->screen.selected),
                 &(data->objectInfo));
      vTaskDelete(NULL);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void wifi_config_callback(void *ctx) {
  DataManager_t *data = (DataManager_t *)ctx;
  ESP_LOGI(TAG_FUNCTION_MANAGER, "WiFi Config callback triggered");
  xTaskCreate(wifi_config_task, "wifi_connect_task", 4096, data, 5, NULL);
}

void information_callback(void *ctx) {}

void read_temperature_cb(void *ctx) {}

void read_humidity_cb(void *ctx) {}

void read_pressure_cb(void *ctx) {}

void read_dht22_cb(void *ctx) {}

void battery_status_callback(void *ctx) {}