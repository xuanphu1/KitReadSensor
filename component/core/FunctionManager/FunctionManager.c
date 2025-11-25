#include "FunctionManager.h"
#include "Common.h"
#include "ScreenManager.h"
#include "SensorConfig.h"
#include "SensorRegistry.h"
#include "freertos/idf_additions.h"
#include <stdbool.h>
#include <stdint.h>

static PortId_t PortSelected[NUM_PORTS] = {PORT_NONE, PORT_NONE, PORT_NONE};
static const char *port_name[3] = {"Port 1", "Port 2", "Port 3"};
static TaskHandle_t readDataSensorTaskHandle[NUM_PORTS];

void wifi_config_task(void *pvParameters) {
  DataManager_t *data = (DataManager_t *)pvParameters;
  wifi_init_ap();

  if (data->objectInfo.wifiInfo.wifiStatus == CONNECTED) {
    data->objectInfo.wifiInfo.wifiStatus = DISCONNECTED;
    ESP_LOGI(TAG_FUNCTION_MANAGER, "WiFi Config callback triggered");
  }
  while (1) {
    ScreenWifiConnecting(data);
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

// Biến global static để chia sẻ giữa các hàm
static char g_port_label_buf[NUM_PORTS][24];

void reset_all_ports_callback(void *ctx) {
  DataManager_t *data = (DataManager_t *)ctx;
  for (PortId_t i = 0; i < NUM_PORTS; i++) {
    data->selectedSensor[i] = SENSOR_NONE;
    PortSelected[i] = PORT_NONE;
    snprintf(g_port_label_buf[i], sizeof(g_port_label_buf[i]), "%s",
             (const char *)port_name[i]);
  }
  sensor_driver_t *drivers = sensor_registry_get_drivers();
  size_t driver_count = sensor_registry_get_count();
  for (size_t i = 0; i < driver_count; i++) {
    drivers[i].is_init = false;
  }

  data->screen.current->items[0].name = g_port_label_buf[0];
  data->screen.current->items[1].name = g_port_label_buf[1];
  data->screen.current->items[2].name = g_port_label_buf[2];
  if (readDataSensorTaskHandle[0] != NULL) {
    vTaskDelete(readDataSensorTaskHandle[0]);
    readDataSensorTaskHandle[0] = NULL;
  }
  if (readDataSensorTaskHandle[1] != NULL) {
    vTaskDelete(readDataSensorTaskHandle[1]);
    readDataSensorTaskHandle[1] = NULL;
  }
  if (readDataSensorTaskHandle[2] != NULL) {
    vTaskDelete(readDataSensorTaskHandle[2]);
    readDataSensorTaskHandle[2] = NULL;
  }
  ESP_LOGI(TAG_FUNCTION_MANAGER, "Reset all ports");
  MenuRender(data->screen.current, &data->screen.selected, &data->objectInfo);
}

void select_sensor_cb(void *ctx) {
  SelectionParam_t *param = (SelectionParam_t *)ctx;

  // Debug: Log ngay khi callback được gọi
  ESP_LOGI(TAG_FUNCTION_MANAGER, "select_sensor_cb called: param=%p", param);
  if (param != NULL) {
    ESP_LOGI(TAG_FUNCTION_MANAGER, "  param->port=%d, param->sensor=%d", 
             param->port, param->sensor);
    ESP_LOGI(TAG_FUNCTION_MANAGER, "  param->data=%p", param->data);
  }

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

  ESP_LOGI(TAG_FUNCTION_MANAGER, "Selected sensor %d (%s) for port %d",
           param->sensor, sensor_type_to_name(param->sensor), param->port);

  // 3. Xử lý khởi tạo sensor và cập nhật label
  sensor_driver_t *driver = sensor_registry_get_driver(param->sensor);
  if (driver == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "select_sensor_cb: invalid sensor type %d",
             param->sensor);
    return;
  }
  Message_t message_type = MESSAGE_NONE;

  if (driver->init != NULL) {
    if (PortSelected[param->port] == param->port) {
      // Port đã được chọn trước đó
      message_type = MESSAGE_PORT_SELECTED;
      ESP_LOGI(TAG_FUNCTION_MANAGER, "Port %d was selected", param->port);
      const char *sensor_name = sensor_type_to_name(param->sensor);
      snprintf(g_port_label_buf[param->port],
               sizeof(g_port_label_buf[param->port]), "Port %d - %s",
               (int)param->port + 1, sensor_name);
    } else if (!driver->is_init) {
      PortSelected[param->port] = param->port; // Cập nhật PortSelected
      // Sensor chưa được khởi tạo
      system_err_t init_ret = driver->init();
      if (init_ret != MRS_OK) {
        ESP_LOGE(TAG_FUNCTION_MANAGER, "select_sensor_cb: init failed for sensor %d: %s",
                 param->sensor, system_err_to_name(init_ret));
        message_type = MESSAGE_SENSOR_NOT_INITIALIZED;
        snprintf(g_port_label_buf[param->port],
                 sizeof(g_port_label_buf[param->port]), "Port %d",
                 (int)param->port + 1);
      } else {
        driver->is_init = true;
      const char *sensor_name = sensor_type_to_name(param->sensor);
      snprintf(g_port_label_buf[param->port],
               sizeof(g_port_label_buf[param->port]), "Port %d - %s",
               (int)param->port + 1, sensor_name);
    } else {
      // Sensor đã được khởi tạo
      PortSelected[param->port] = param->port;
      ESP_LOGI(TAG_FUNCTION_MANAGER,
               "select_sensor_cb: sensor %d is already initialized",
               param->sensor);
      message_type = MESSAGE_SENSOR_USED_OTHER_PORT;
      const char *sensor_name = sensor_type_to_name(param->sensor);
      snprintf(g_port_label_buf[param->port],
               sizeof(g_port_label_buf[param->port]), "Port %d - %s",
               (int)param->port + 1, sensor_name);
    }
  } else {
    // Sensor không có hàm init
    ESP_LOGW(TAG_FUNCTION_MANAGER,
             "select_sensor_cb: init is NULL for sensor %d", param->sensor);
    message_type = MESSAGE_SENSOR_NOT_INITIALIZED;
    snprintf(g_port_label_buf[param->port],
             sizeof(g_port_label_buf[param->port]), "Port %d",
             (int)param->port + 1);
  }

  // 4. Hiển thị message nếu cần
  if (message_type != MESSAGE_NONE) {
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

void readDataSensorTask(void *pvParameters) {
  ShowDataSensorParam_t *param = (ShowDataSensorParam_t *)pvParameters;
  if (param == NULL || param->data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "readDataSensorTask: data is NULL");
    return;
  }
  while (1) {
    SensorData_t data;
    sensor_driver_t *driver = sensor_registry_get_driver(
        param->data->selectedSensor[param->port]);
    if (driver == NULL || driver->read == NULL) {
      ESP_LOGW(TAG_FUNCTION_MANAGER, "readDataSensorTask: invalid driver");
      ScreenShowMessage(MESSAGE_SENSOR_NOT_INITIALIZED);
      vTaskDelay(pdMS_TO_TICKS(1000));
      MenuRender(param->data->screen.current, &param->data->screen.selected,
        &param->data->objectInfo);
      readDataSensorTaskHandle[param->port] = NULL;
      vTaskDelete(readDataSensorTaskHandle[param->port]);
      vTaskDelete(NULL);
      return;
    }
    
    system_err_t read_ret = driver->read(&data);
    if (read_ret != MRS_OK) {
      ESP_LOGW(TAG_FUNCTION_MANAGER, "readDataSensorTask: read failed: %s",
               system_err_to_name(read_ret));
      // Tiếp tục loop để thử lại
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    const char **field_names = driver->description;
    const char **units = driver->unit;
    size_t count = driver->unit_count;
    if (param->ShowDataScreen) {
      ScreenShowDataSensor(field_names, data.data_fl, units, count);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void show_data_sensor_cb(void *ctx) {
  ShowDataSensorParam_t *param = (ShowDataSensorParam_t *)ctx;
  param->ShowDataScreen = true;
  if (param == NULL || param->data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "show_data_sensor_cb: data is NULL");
    return;
  }
  ESP_LOGI(TAG_FUNCTION_MANAGER, "Port %d read sensor %s", param->port,
           sensor_type_to_name(param->data->selectedSensor[param->port]));
  if (readDataSensorTaskHandle[param->port] == NULL) {
    ESP_LOGI(TAG_FUNCTION_MANAGER, "Task is running");
    xTaskCreate(readDataSensorTask, "readDataSensorTask", 4096, param, 5,
                &readDataSensorTaskHandle[param->port]);
  }
}
