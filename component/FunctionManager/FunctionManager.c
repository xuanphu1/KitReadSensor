#include "FunctionManager.h"
#include "ScreenManager.h"
#include "SensorSystem.h"
#include <stdbool.h>



static uint8_t oldSelectedSensor[NUM_PORTS];

sensor_driver_t sensor_drivers[] = {
    {
        .name = "BME280",
        .init = bme280Init,
        .read = bme280Read,
        .deinit = bme280Deinit,
        .unit = {"°C", "hPa", "%"},
        .unit_count = 3,
        .is_init = false,
    },
    {
        .name = "MH-Z14A",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .unit = {"ppm", "ppm", "ppm"},
        .unit_count = 3,
        .is_init = false,
    },
    {
        .name = "PMS7003",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .unit = {"ug/m3", "ug/m3", "ug/m3"},
        .unit_count = 3,
        .is_init = false,
    },
    {
        .name = "DHT22",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .unit = {"°C", "%", "hPa"},
        .unit_count = 3,
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


static bool trackingInitSensor(DataManager_t *data, PortId_t port) {
  if (oldSelectedSensor[port] != data->selectedSensor[port] && data->selectedSensor[port] != SENSOR_NONE) {
    return true;
  }
  return false;
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
  ESP_LOGI(TAG_FUNCTION_MANAGER,"Check Init Sensor: %d", sensor_drivers[param->sensor].is_init);
  param->data->selectedSensor[param->port] = (int8_t)param->sensor;
  ESP_LOGI(TAG_FUNCTION_MANAGER, "Selected sensor %d for port %d",
           param->sensor, param->port);

  if (sensor_drivers[param->sensor].init != NULL) {
    if (!sensor_drivers[param->sensor].is_init) {
      sensor_drivers[param->sensor].init();
      sensor_drivers[param->sensor].is_init = true;
      // Cập nhật nhãn hiển thị cho mục Port X
      const char *sensor_name = sensor_type_to_name(param->sensor);
      snprintf(g_port_label_buf[param->port],
               sizeof(g_port_label_buf[param->port]), "Port %d - %s",
               (int)param->port + 1, sensor_name);

    } else {
      ESP_LOGI(TAG_FUNCTION_MANAGER,
               "select_sensor_cb: sensor %d is already initialized",
               param->sensor);
      snprintf(g_port_label_buf[param->port],
               sizeof(g_port_label_buf[param->port]), "Port %d",
               (int)param->port + 1);
      ScreenShowMessage(0);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  } else {
    ESP_LOGW(TAG_FUNCTION_MANAGER,
             "select_sensor_cb: init is NULL for sensor %d", param->sensor);
      snprintf(g_port_label_buf[param->port],
               sizeof(g_port_label_buf[param->port]), "Port %d",
               (int)param->port + 1);
      ScreenShowMessage(1);
      vTaskDelay(pdMS_TO_TICKS(1000));
  }
  // Quay lại menu SensorPort và render lại, chọn đúng dòng theo port
  if (param->data->screen.current && param->data->screen.current->parent) {
    param->data->screen.current = param->data->screen.current->parent;
    param->data->screen.selected =
        (int8_t)param->port; // highlight port vừa chọn
    param->data->screen.current->items[param->port].name =
        g_port_label_buf[param->port];
    ESP_LOGI(TAG_FUNCTION_MANAGER, "Name of menu item: %s",
             param->data->screen.current->items[param->port].name);
  } else {
    ESP_LOGW(TAG_FUNCTION_MANAGER,
             "select_sensor_cb: current or parent is NULL");
  }
  oldSelectedSensor[param->port] = param->sensor;
  MenuRender(param->data->screen.current, &param->data->screen.selected,
             &param->data->objectInfo);
}

// Hiển thị live dữ liệu cảm biến cho từng port (BME280 trước)
static void sensor_live_cb(void *ctx);

void start_read_sensor_cb(void *ctx) {
  DataManager_t *data = (DataManager_t *)ctx;
  if (data == NULL) {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "start_read_sensor_cb: data is NULL");
    return;
  }

  // Khởi tạo các cảm biến đã chọn (hiện hỗ trợ BME280)
  // bool need_bme = false;
  // for (int p = 0; p < NUM_PORTS; ++p) {
  //   if (data->selectedSensor[p] == SENSOR_BME280) need_bme = true;
  // }
  // static bool bme_inited = false;
  // if (need_bme && !bme_inited) {
  //   bme280Init();
  //   bme_inited = true;
  //   ESP_LOGI(TAG_FUNCTION_MANAGER, "BME280 initialized");
  // }

  // // Tạo menu hiển thị live cho 3 port
  // typedef struct { DataManager_t *data; PortId_t port; } LiveParam_t;
  // static LiveParam_t live_params[NUM_PORTS];
  // static char live_label_buf[NUM_PORTS][32];
  // static menu_item_t live_items[NUM_PORTS];
  // static menu_list_t live_menu;

  // for (int p = 0; p < NUM_PORTS; ++p) {
  //   const char *sname =
  //   sensor_type_to_name((SensorType_t)data->selectedSensor[p]);
  //   snprintf(live_label_buf[p], sizeof(live_label_buf[p]), "Port %d - %s",
  //   p+1, sname); live_params[p].data = data; live_params[p].port =
  //   (PortId_t)p; live_items[p].name = live_label_buf[p]; live_items[p].type =
  //   MENU_ACTION; live_items[p].callback = sensor_live_cb; live_items[p].ctx =
  //   &live_params[p]; live_items[p].children = NULL;
  // }
  // live_menu.items = live_items;
  // live_menu.count = NUM_PORTS;
  // live_menu.parent = data->screen.current; // quay lại menu trước đó
  // live_menu.object = OBJECT_SENSOR;

  // data->screen.current = &live_menu;
  // data->screen.selected = 0;
  // MenuRender(data->screen.current, &data->screen.selected,
  // &data->objectInfo);
}

static void sensor_live_cb(void *ctx) {
  typedef struct {
    DataManager_t *data;
    PortId_t port;
  } LiveParam_t;
  LiveParam_t *param = (LiveParam_t *)ctx;
  if (!param || !param->data)
    return;
  DataManager_t *data = param->data;
  int8_t sel = data->selectedSensor[param->port];
  if (sel == SENSOR_BME280) {
    bme280Read(&data->sensor);
    float t = data->sensor.data_fl[0];
    float h = data->sensor.data_fl[1];
    float p = data->sensor.data_fl[2];
    // Cập nhật label của item để hiển thị nhanh giá trị
    menu_list_t *menu = data->screen.current;
    if (!menu)
      return;
    static char value_buf[NUM_PORTS][32];
    snprintf(value_buf[param->port], sizeof(value_buf[param->port]),
             "Port %d - T%.1f H%.1f P%.0f", (int)param->port + 1, t, h, p);
    menu->items[param->port].name = value_buf[param->port];
    MenuRender(menu, &data->screen.selected, &data->objectInfo);
  } else {
    ESP_LOGW(TAG_FUNCTION_MANAGER, "sensor_live_cb: unsupported sensor type %d",
             sel);
  }
}