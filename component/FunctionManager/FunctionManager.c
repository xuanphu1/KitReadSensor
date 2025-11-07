#include "FunctionManager.h"
#include "DataManager.h"
#include "ScreenManager.h"
#include "SensorSystem.h"
#include "freertos/idf_additions.h"
#include <stdbool.h>
#include <stdint.h>

static PortId_t PortSelected[NUM_PORTS] = {PORT_NONE, PORT_NONE, PORT_NONE};
void TestInit(void) { ESP_LOGI(TAG_FUNCTION_MANAGER, "TestInit"); }
static const char *port_name[3] = {"Port 1", "Port 2", "Port 3"};

sensor_driver_t sensor_drivers[] = {
    {
        .name = "BME280",
        .init = bme280Init,
        .read = bme280Read,
        .deinit = bme280Deinit,
        .description = {"Temperature", "Pressure", "Humidity"},
        .unit = {"°C", "hPa", "%"},
        .unit_count = 3,
        .is_init = false,
    },
    {
        .name = "MH-Z14A",
        .init = TestInit,
        .read = NULL,
        .deinit = NULL,
        .description = {"CO2"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "PMS7003",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"PM1.0", "PM2.5", "PM10"},
        .unit = {"ug/m3", "ug/m3", "ug/m3"},
        .unit_count = 3,
        .is_init = false,
    },
    {
        .name = "DHT22",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"Temperature", "Humidity"},
        .unit = {"°C", "%"},
        .unit_count = 2,
        .is_init = false,
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

void reset_all_ports_callback(void *ctx) {
  DataManager_t *data = (DataManager_t *)ctx;
  static char g_port_label_buf[NUM_PORTS][24];
  for (PortId_t i = 0; i < NUM_PORTS; i++) {
    data->selectedSensor[i] = SENSOR_NONE;
    sensor_drivers[i].is_init = false;
    PortSelected[i] = PORT_NONE;
    snprintf(g_port_label_buf[i], sizeof(g_port_label_buf[i]), "%s",
             (const char *)port_name[i]);
  }

  data->screen.current->items[0].name = g_port_label_buf[0];
  data->screen.current->items[1].name = g_port_label_buf[1];
  data->screen.current->items[2].name = g_port_label_buf[2];

  ESP_LOGI(TAG_FUNCTION_MANAGER, "Reset all ports");
  MenuRender(data->screen.current, &data->screen.selected, &data->objectInfo);
}

// bộ đệm nhãn cho 3 port (được trỏ từ MenuSystem qua
// SelectionParam_t.sensorName)

void select_sensor_cb(void *ctx) {
  static char g_port_label_buf[NUM_PORTS][24];
  SelectionParam_t *param = (SelectionParam_t *)ctx;

  // 1. Validation
  if (param == NULL || param->data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "select_sensor_cb: invalid ctx");
    return;
  }
  if (param->port < 0 || param->port >= NUM_PORTS) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "select_sensor_cb: invalid port %d",
             param->port);
    return;
  }

  // 2. Lưu lựa chọn
  param->data->selectedSensor[param->port] = (int8_t)param->sensor;

  ESP_LOGI(TAG_FUNCTION_MANAGER, "Selected sensor %d for port %d",
           param->sensor, param->port);

  // 3. Xử lý khởi tạo sensor và cập nhật label
  sensor_driver_t *driver = &sensor_drivers[param->sensor];
  int message_type = -1;

  if (driver->init != NULL) {
    if (PortSelected[param->port] == param->port) {
      // Port đã được chọn trước đó
      message_type = 2;
      ESP_LOGI(TAG_FUNCTION_MANAGER, "Port %d was selected", param->port);
    } else if (!driver->is_init) {
      PortSelected[param->port] = param->port; // Cập nhật PortSelected
      // Sensor chưa được khởi tạo
      driver->init();
      driver->is_init = true;
      const char *sensor_name = sensor_type_to_name(param->sensor);
      snprintf(g_port_label_buf[param->port],
               sizeof(g_port_label_buf[param->port]), "Port %d - %s",
               (int)param->port + 1, sensor_name);
    } else {
      // Sensor đã được khởi tạo
      ESP_LOGI(TAG_FUNCTION_MANAGER,
               "select_sensor_cb: sensor %d is already initialized",
               param->sensor);
      message_type = 0;
      snprintf(g_port_label_buf[param->port],
               sizeof(g_port_label_buf[param->port]), "Port %d",
               (int)param->port + 1);
    }
  } else {
    // Sensor không có hàm init
    ESP_LOGW(TAG_FUNCTION_MANAGER,
             "select_sensor_cb: init is NULL for sensor %d", param->sensor);
    message_type = 1;
    snprintf(g_port_label_buf[param->port],
             sizeof(g_port_label_buf[param->port]), "Port %d",
             (int)param->port + 1);
  }

  // 4. Hiển thị message nếu cần
  if (message_type >= 0) {
    ScreenShowMessage(message_type);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // 5. Quay lại menu parent và render
  if (param->data->screen.current && param->data->screen.current->parent) {
    param->data->screen.current = param->data->screen.current->parent;
    param->data->screen.selected = (int8_t)param->port;
    param->data->screen.current->items[param->port].name =
        g_port_label_buf[param->port];
    ESP_LOGI(TAG_FUNCTION_MANAGER, "Name of menu item: %s",
             param->data->screen.current->items[param->port].name);
  } else {
    ESP_LOGW(TAG_FUNCTION_MANAGER,
             "select_sensor_cb: current or parent is NULL");
  }

  MenuRender(param->data->screen.current, &param->data->screen.selected,
             &param->data->objectInfo);
}

TaskHandle_t readDataSensorTaskHandle[NUM_PORTS];


void readDataSensorTask(void *pvParameters) {
  ShowDataSensorParam_t *param = (ShowDataSensorParam_t *)pvParameters;
  if (param == NULL || param->data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "readDataSensorTask: data is NULL");
    return;
  }
  while (1) {
    SensorData_t data;
    sensor_driver_t *driver =
        &sensor_drivers[param->data->selectedSensor[param->port]];
    driver->read(&data);
    const char **field_names = driver->description;
    const char **units = driver->unit;
    size_t count = driver->unit_count;
    ScreenShowDataSensor(field_names, data.data_fl, units, count);
    // ESP_LOGI(TAG_FUNCTION_MANAGER, "Read data sensor %s", driver->name);
    // for (size_t i = 0; i < count; i++) {
    //   ESP_LOGI(TAG_FUNCTION_MANAGER, "Data %s: %f", field_names[i], data.data_fl[i]);
    // }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void show_data_sensor_cb(void *ctx) {
  ShowDataSensorParam_t *param = (ShowDataSensorParam_t *)ctx;
  if (param == NULL || param->data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "show_data_sensor_cb: data is NULL");
    return;
  }
  ESP_LOGI(TAG_FUNCTION_MANAGER, "Port %d read sensor %s", param->port,
           sensor_type_to_name(param->data->selectedSensor[param->port]));
  xTaskCreate(readDataSensorTask, "readDataSensorTask", 4096, param, 5,
              &readDataSensorTaskHandle[param->port]);
}
