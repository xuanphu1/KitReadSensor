#include "FunctionManager.h"
#include "Common.h"
#include "ScreenManager.h"
#include "SensorConfig.h"
#include "SensorRegistry.h"
#include "WifiManager.h"
#include "BatteryManager.h"
#include "freertos/idf_additions.h"
#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

static PortId_t PortSelected[NUM_PORTS] = {PORT_NONE, PORT_NONE, PORT_NONE};
static const char *port_name[NUM_PORTS] = {"Port 1", "Port 2", "Port 3"};
static TaskHandle_t readDataSensorTaskHandle[NUM_PORTS];

void wifi_config_task(void *pvParameters) {
  DataManager_t *data = (DataManager_t *)pvParameters;
  system_err_t ret = wifi_init_ap();
  if (ret != MRS_OK) {
    ESP_LOGE(TAG_FUNCTION_MANAGER, "Failed to initialize WiFi AP: %s",
             system_err_to_name(ret));
  }

  if (data->objectInfo.wifiInfo.wifiStatus == CONNECTED) {
    data->objectInfo.wifiInfo.wifiStatus = DISCONNECTED;
    ESP_LOGI(TAG_FUNCTION_MANAGER, "WiFi Config callback triggered");
  }
  while (1) {
    ScreenWifiConnecting(data);
    ret = update_wifi_status(&(data->objectInfo.wifiInfo));
    if (ret != MRS_OK) {
      ESP_LOGW(TAG_FUNCTION_MANAGER, "Failed to update WiFi status: %s",
               system_err_to_name(ret));
    }

    ret = wifi_connect_handler(data);
    if (ret != MRS_OK && ret != MRS_ERR_NETWORK_TIMEOUT) {
      ESP_LOGW(TAG_FUNCTION_MANAGER, "WiFi connect handler error: %s",
               system_err_to_name(ret));
    }

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


/* -------------------- Actuators (GPIO output) -------------------- */
static void actuator_set_level(void *ctx, int level) {
  int gpio_num = (int)(uintptr_t)ctx;
  gpio_config_t io = {
    .pin_bit_mask = (1ULL << gpio_num),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  if (gpio_config(&io) != ESP_OK) {
    ESP_LOGE(TAG_FUNCTION_MANAGER, "Actuator GPIO %d config failed", gpio_num);
    return;
  }
  gpio_set_level((gpio_num_t)gpio_num, level);
  ESP_LOGI(TAG_FUNCTION_MANAGER, "Actuator GPIO %d -> %s", gpio_num, level ? "ON" : "OFF");
}


void actuator_on_cb(void *ctx) {
  actuator_set_level(ctx, 1);
}

void actuator_off_cb(void *ctx) {
  actuator_set_level(ctx, 0);
}

void information_callback(void *ctx) {
  DataManager_t *data = (DataManager_t *)ctx;
  if (data == NULL) {
    return;
  }
  static const char *info_lines[] = {
    "MSMS Project",
    "Module quan ly",
    "cam bien da nang",
    "Tac gia: MrKoi",
  };
  ScreenShowInformation(info_lines, sizeof(info_lines) / sizeof(info_lines[0]));
  vTaskDelay(pdMS_TO_TICKS(4000));
  /* Quay lai hien thi Root menu */
  MenuRender(data->screen.current, &data->screen.selected, &data->objectInfo);
}

void read_temperature_cb(void *ctx) {}

void read_humidity_cb(void *ctx) {}

void read_pressure_cb(void *ctx) {}

void read_dht22_cb(void *ctx) {}

void battery_status_callback(void *ctx) {
  DataManager_t *data = (DataManager_t *)ctx;
  if (data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "battery_status_callback: data is NULL");
    return;
  }
  
  ESP_LOGI(TAG_FUNCTION_MANAGER, "Battery Status callback triggered");
  
  // Cập nhật thông tin pin từ BatteryManager
  BatteryManager_UpdateInfo(&(data->objectInfo.batteryInfo));
  
  // Hiển thị menu Battery Status
  if (data->MenuReturn[1] != NULL) {
    MenuRender(data->MenuReturn[1], &(data->screen.selected), &(data->objectInfo));
  }
}

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

  if (data->on_ports_reset) {
    data->on_ports_reset(data);
  }
  /* Cập nhật tên mục trong menu hiện tại (vd Show data sensor) nếu cần */
  for (PortId_t i = 0; i < NUM_PORTS; i++) {
    data->screen.current->items[i].name = g_port_label_buf[i];
  }
  
  // Dừng và xóa các task đọc sensor đang chạy
  for (PortId_t i = 0; i < NUM_PORTS; i++) {
    if (readDataSensorTaskHandle[i] != NULL) {
      vTaskDelete(readDataSensorTaskHandle[i]);
      readDataSensorTaskHandle[i] = NULL;
    }
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

  ESP_LOGI(TAG_FUNCTION_MANAGER, "Selected sensor %d (%s) for port %d",
           param->sensor, sensor_type_to_name(param->sensor), param->port);

  // 2. Lấy driver và kiểm tra init; chỉ gán selectedSensor khi init thành công (hoặc đã init)
  sensor_driver_t *driver = sensor_registry_get_driver(param->sensor);
  if (driver == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "select_sensor_cb: invalid sensor type %d",
             param->sensor);
    return;
  }
  Message_t message_type = MESSAGE_NONE;
  bool allow_set_sensor = false;  // Chỉ set selectedSensor[port] khi true

  if (driver->init != NULL) {
    if (PortSelected[param->port] == param->port) {
      message_type = MESSAGE_PORT_SELECTED;
      ESP_LOGI(TAG_FUNCTION_MANAGER, "Port %d was selected", param->port);
      allow_set_sensor = true;
    } else if (!driver->is_init) {
      system_err_t init_ret = driver->init();
      if (init_ret != MRS_OK) {
        ESP_LOGE(TAG_FUNCTION_MANAGER,
                 "select_sensor_cb: init failed for sensor %d: %s",
                 param->sensor, system_err_to_name(init_ret));
        message_type = MESSAGE_SENSOR_NOT_INITIALIZED;
        /* Không gán selectedSensor -> giữ "Port X" */
      } else {
        driver->is_init = true;
        PortSelected[param->port] = param->port;
        allow_set_sensor = true;
      }
    } else {
      PortSelected[param->port] = param->port;
      ESP_LOGI(TAG_FUNCTION_MANAGER,
               "select_sensor_cb: sensor %d is already initialized",
               param->sensor);
      message_type = MESSAGE_SENSOR_USED_OTHER_PORT;
      allow_set_sensor = true;
    }
  } else {
    ESP_LOGW(TAG_FUNCTION_MANAGER,
             "select_sensor_cb: init is NULL for sensor %d", param->sensor);
    message_type = MESSAGE_SENSOR_NOT_INITIALIZED;
    /* Không có init -> không coi là đã chọn cảm biến, giữ "Port X" */
  }

  if (allow_set_sensor) {
    param->data->selectedSensor[param->port] = (int8_t)param->sensor;
  } else {
    param->data->selectedSensor[param->port] = (int8_t)SENSOR_NONE;
  }

  // 4. Hiển thị message nếu cần
  if (message_type != MESSAGE_NONE) {
    ScreenShowMessage(message_type);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // 5. Cập nhật tên Port và quay về menu Sensors (MenuSystem thực hiện qua callback)
  if (param->data->on_sensor_selected) {
    param->data->on_sensor_selected(param->data, param->port);
  }
}

void readDataSensorTask(void *pvParameters) {
  ShowDataSensorParam_t *param = (ShowDataSensorParam_t *)pvParameters;
  if (param == NULL || param->data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "readDataSensorTask: data is NULL");
    return;
  }
  while (1) {
    SensorData_t data;
    sensor_driver_t *driver =
        sensor_registry_get_driver(param->data->selectedSensor[param->port]);
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
