#include "FunctionManager.h"




sensor_driver_t sensor_drivers[] = {
  {
    .name = "BME280",
    .init = bme280Init,
    .read = bme280Read,
    .deinit = bme280Deinit,
    .unit = {"°C", "hPa", "%"},
    .unit_count = 3,
  },
};



void wifi_config_task(void *pvParameters) {
  DataManager_t *data = (DataManager_t *)pvParameters;
  wifi_init_ap();

  if (data->objectInfo.wifiInfo.wifiStatus == CONNECTED) {
    data->objectInfo.wifiInfo.wifiStatus = DISCONNECTED;
    ESP_LOGI(TAG_FUNCTION_MANAGER, "WiFi Config callback triggered");
  }
  while (1) {
    ScreenWifiCallback(data);
    update_wifi_status(&(data->objectInfo.wifiInfo));
    wifi_connect_handler(data);
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

static const char *sensor_type_to_name(SensorType_t t) {
  switch (t) {
  case SENSOR_BME280:
    return "BME280";
  case SENSOR_MHZ14A:
    return "MH-Z14A";
  case SENSOR_PMS7003:
    return "PMS7003";
  case SENSOR_DHT22:
    return "DHT22";
  default:
    return "Unknown";
  }
}

// bộ đệm nhãn cho 3 port (được trỏ từ MenuSystem qua
// SelectionParam_t.sensorName)
static char g_port_label_buf[NUM_PORTS][24];

void select_sensor_cb(void *ctx) {
  SelectionParam_t *param = (SelectionParam_t *)ctx;
  if (param == NULL || param->data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "select_sensor_cb: invalid ctx");
    return;
  }
  if (param->port < 0 || param->port >= NUM_PORTS) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "select_sensor_cb: invalid port %d",
             param->port);
    return;
  }
  param->data->selectedSensor[param->port] = (int8_t)param->sensor;
  ESP_LOGI(TAG_FUNCTION_MANAGER, "Selected sensor %d for port %d",
           param->sensor, param->port);

  // Cập nhật nhãn hiển thị cho mục Port X
  const char *sensor_name = sensor_type_to_name(param->sensor);
  snprintf(g_port_label_buf[param->port], sizeof(g_port_label_buf[param->port]),
           "Port %d - %s", (int)param->port + 1, sensor_name);
  // Quay lại menu SensorPort và render lại, chọn đúng dòng theo port
  if (param->data->screen.current && param->data->screen.current->parent) {
    param->data->screen.current = param->data->screen.current->parent;
    param->data->screen.selected =
        (int8_t)param->port; // highlight port vừa chọn
    param->data->screen.current->items[param->port].name =
        g_port_label_buf[param->port];
    ESP_LOGI(TAG_FUNCTION_MANAGER, "Name of menu item: %s",
             param->data->screen.current->items[param->port].name);
    MenuRender(param->data->screen.current, &param->data->screen.selected,
               &param->data->objectInfo);
  } else {
    ESP_LOGW(TAG_FUNCTION_MANAGER,
             "select_sensor_cb: current or parent is NULL");
  }
}

void start_read_sensor_cb(void *ctx) {
  DataManager_t *data = (DataManager_t *)ctx;
  if (data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "start_read_sensor_cb: data is NULL");
    return;
  }
  for (int p = 0; p < NUM_PORTS; ++p) {
    int8_t selected = data->selectedSensor[p];
    ESP_LOGI(TAG_FUNCTION_MANAGER, "Port %d -> sensor %d", p, selected);
    // Chỗ này có thể gọi driver tương ứng theo selected
  }
}