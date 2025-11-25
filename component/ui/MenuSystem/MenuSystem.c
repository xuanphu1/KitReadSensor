#include "MenuSystem.h"
#include "Common.h"
#include "SensorRegistry.h"
#include <stdbool.h>
#include <sys/_types.h>
#include <string.h>
#include <stdlib.h>

menu_list_t WiFi_Config_Menu;
DataManager_t *Data;
static const char *port[3] = {"Port 1", "Port 2", "Port 3"};

menu_list_t Sensor_Port_1;
menu_list_t Sensor_Port_2;
menu_list_t Sensor_Port_3;

/* -------------------- Dynamic Arrays -------------------- */
// Mảng động được cấp phát dựa trên số lượng sensor thực tế từ registry
// Cấu trúc: [NUM_PORTS][sensor_count] - mỗi port có mảng riêng
static SelectionParam_t *SensorSelection[NUM_PORTS] = {NULL, NULL, NULL};
static menu_item_t *Sensor_Port_Items[NUM_PORTS] = {NULL, NULL, NULL};
static size_t g_sensor_count = 0;  // Số lượng sensor thực tế

ShowDataSensorParam_t ShowDataSensorSelection[NUM_PORTS];

/* -------------------- Menu Tree -------------------- */
// Menu lists cho các port (sẽ được khởi tạo động với count thực tế)
// Lưu ý: items sẽ được cập nhật trong init_sensor_port_menu_items()
menu_list_t Sensor_Port_1 = {
    .items = NULL,  // Sẽ được cập nhật trong init_sensor_port_menu_items()
    .count = 0,
    .parent = NULL,
};
menu_list_t Sensor_Port_2 = {
    .items = NULL,  // Sẽ được cập nhật trong init_sensor_port_menu_items()
    .count = 0,
    .parent = NULL,
};
menu_list_t Sensor_Port_3 = {
    .items = NULL,  // Sẽ được cập nhật trong init_sensor_port_menu_items()
    .count = 0,
    .parent = NULL,
};
menu_item_t PortConfig[] = {
    {NULL, MENU_SUBMENU, NULL, NULL, &Sensor_Port_1},
    {NULL, MENU_SUBMENU, NULL, NULL, &Sensor_Port_2},
    {NULL, MENU_SUBMENU, NULL, NULL, &Sensor_Port_3},
    {"Reset All Ports", MENU_ACTION, reset_all_ports_callback, NULL, NULL},
};

menu_list_t Sensor_Handler = {
    .items = PortConfig,
    .count = ARRAY_SIZE(PortConfig),
    .parent = NULL,
};

menu_item_t Show_Data_Sensor[] = {{NULL, MENU_ACTION, show_data_sensor_cb,
                                   &ShowDataSensorSelection[PORT_1], NULL},
                                  {NULL, MENU_ACTION, show_data_sensor_cb,
                                   &ShowDataSensorSelection[PORT_2], NULL},
                                  {NULL, MENU_ACTION, show_data_sensor_cb,
                                   &ShowDataSensorSelection[PORT_3], NULL}};

menu_list_t Show_Data_Sensor_Menu = {
    .items = Show_Data_Sensor,
    .count = ARRAY_SIZE(Show_Data_Sensor),
    .parent = NULL,
};

menu_item_t Sensor_Menu_Items[] = {
    {"Port Config", MENU_SUBMENU, NULL, NULL, &Sensor_Handler},
    {"Show data sensor", MENU_SUBMENU, NULL, NULL, &Show_Data_Sensor_Menu}};

menu_list_t Sensor_Menu = {
    .items = Sensor_Menu_Items,
    .count = ARRAY_SIZE(Sensor_Menu_Items),
    .parent = NULL, // gán sau
};

// Submenu "WiFi Config"
menu_item_t WiFi_Config_Items[] = {
    {"OK", MENU_ACTION, wifi_config_callback, NULL, NULL},
};

const image_t WiFi_Config_Image = {
    .image = imageManager,
    .width = 19,
    .height = 16,
};

const text_t WiFi_Config_Text = {
    .size = 12,
    .text = ManagerText,
};

menu_list_t WiFi_Config_Menu = {
    .items = WiFi_Config_Items,
    .text = WiFi_Config_Text,
    .image = WiFi_Config_Image,
    .object = OBJECT_WIFI,
    .count = ARRAY_SIZE(WiFi_Config_Items),
    .parent = NULL,
};

// Root Menu
menu_item_t Root_Items[] = {
    {"WiFi Config", MENU_SUBMENU, NULL, NULL, &WiFi_Config_Menu},
    {"Sensors", MENU_SUBMENU, NULL, NULL, &Sensor_Menu},
    {"Battery Status", MENU_ACTION, battery_status_callback, NULL, NULL},
    {"Information", MENU_ACTION, information_callback, NULL, NULL},
};

menu_list_t Root_Menu = {
    .items = Root_Items,
    .text = {NULL, 0},
    .image = {NULL, 0, 0},
    .object = OBJECT_NONE,
    .count = ARRAY_SIZE(Root_Items),
    .parent = NULL,
};

// Liên kết parent cho submenu
__attribute__((constructor)) static void link_menus(void) {
  Sensor_Menu.parent = &Root_Menu;
  WiFi_Config_Menu.parent = &Root_Menu;
  Sensor_Handler.parent = &Sensor_Menu;
  Show_Data_Sensor_Menu.parent = &Sensor_Menu;
  Sensor_Port_1.parent = &Sensor_Handler;
  Sensor_Port_2.parent = &Sensor_Handler;
  Sensor_Port_3.parent = &Sensor_Handler;
}

/* -------------------- Menu Functions -------------------- */

/**
 * @brief Khởi tạo menu items cho sensor selection từ SensorRegistry
 * 
 * Hàm này tự động tạo menu items cho mỗi port dựa trên danh sách sensor
 * đã đăng ký trong SensorRegistry. Khi thêm sensor mới, chỉ cần thêm vào
 * SensorRegistry, menu sẽ tự động cập nhật.
 * 
 * Sử dụng cấp phát động dựa trên số lượng sensor thực tế từ registry.
 */
static void init_sensor_port_menu_items(DataManager_t *data) {
  sensor_driver_t *drivers = sensor_registry_get_drivers();
  size_t driver_count = sensor_registry_get_count();

  if (driver_count == 0) {
    ESP_LOGE(TAG_MENU_SYSTEM, "No sensors registered in registry!");
    return;
  }

  g_sensor_count = driver_count;

  // Cấp phát động cho mỗi port
  for (PortId_t port = PORT_1; port < NUM_PORTS; port++) {
    // Giải phóng bộ nhớ cũ nếu có
    if (SensorSelection[port] != NULL) {
      free(SensorSelection[port]);
      SensorSelection[port] = NULL;
    }
    if (Sensor_Port_Items[port] != NULL) {
      free(Sensor_Port_Items[port]);
      Sensor_Port_Items[port] = NULL;
    }

    // Cấp phát động dựa trên số lượng sensor thực tế
    SensorSelection[port] = (SelectionParam_t *)malloc(driver_count * sizeof(SelectionParam_t));
    Sensor_Port_Items[port] = (menu_item_t *)malloc(driver_count * sizeof(menu_item_t));

    if (SensorSelection[port] == NULL || Sensor_Port_Items[port] == NULL) {
      ESP_LOGE(TAG_MENU_SYSTEM, "Failed to allocate memory for port %d!", port);
      continue;
    }

    // Khởi tạo menu items cho port này
    for (size_t sensor_idx = 0; sensor_idx < driver_count; sensor_idx++) {
      SensorType_t sensor_type = (SensorType_t)sensor_idx;
      sensor_driver_t *driver = &drivers[sensor_idx];

      // Khởi tạo SelectionParam
      SensorSelection[port][sensor_idx] = (SelectionParam_t){
          .data = data,
          .port = port,
          .sensor = sensor_type
      };

      // Khởi tạo menu item
      Sensor_Port_Items[port][sensor_idx] = (menu_item_t){
          .name = driver->name,                    // Tên sensor từ registry
          .type = MENU_ACTION,
          .callback = select_sensor_cb,
          .ctx = &SensorSelection[port][sensor_idx],
          .children = NULL
      };
    }
  }

  // Cập nhật menu lists với con trỏ và count
  Sensor_Port_1.items = Sensor_Port_Items[PORT_1];
  Sensor_Port_1.count = driver_count;
  Sensor_Port_2.items = Sensor_Port_Items[PORT_2];
  Sensor_Port_2.count = driver_count;
  Sensor_Port_3.items = Sensor_Port_Items[PORT_3];
  Sensor_Port_3.count = driver_count;
}

static void init_show_data_sensor_selection(DataManager_t *data) {
  for (PortId_t port = PORT_1; port < NUM_PORTS; port++) {
    ShowDataSensorSelection[port] = (ShowDataSensorParam_t){
        .data = data, .port = port, .ShowDataScreen = false};
  }
}

void MenuSystemInit(DataManager_t *data) {
  Data = data;
  Data->screen.current = &Root_Menu;
  Data->screen.selected = 0;
  Data->screen.prev_selected = 0;
  Data->MenuReturn[0] = &WiFi_Config_Menu;
  // Khởi tạo selectedSensor
  for (int i = 0; i < NUM_PORTS; ++i) {
    Data->selectedSensor[i] = SENSOR_NONE;
  }
  
  // Khởi tạo menu items động từ SensorRegistry
  init_sensor_port_menu_items(Data);
  init_show_data_sensor_selection(Data);

  PortConfig[0].name = port[0];
  PortConfig[1].name = port[1];
  PortConfig[2].name = port[2];
  PortConfig[3].ctx = Data;

  Show_Data_Sensor[0].name = port[0];
  Show_Data_Sensor[1].name = port[1];
  Show_Data_Sensor[2].name = port[2];

  WiFi_Config_Items[0].ctx = Data;
  Sensor_Menu_Items[1].ctx = Data;
}

/* -------------------- Navigation Task -------------------- */
void MenuNavigation_Task(void *pvParameter) {
  DataManager_t *data = (DataManager_t *)pvParameter;

  data->screen.current = &Root_Menu;
  data->screen.selected = 0;
  data->screen.prev_selected = 0;

  MenuRender(data->screen.current, &data->screen.selected, &data->objectInfo);

  while (1) {
    switch (ReadButtonStatus()) {
    case BTN_UP:
      data->screen.prev_selected = data->screen.selected;
      data->screen.selected--;

      if (data->screen.selected < 0)
        data->screen.selected = data->screen.current->count - 1;
      MenuRender(data->screen.current, &data->screen.selected,
                 &data->objectInfo);
      break;

    case BTN_DOWN:
      data->screen.prev_selected = data->screen.selected;
      data->screen.selected++;

      if (data->screen.selected >= data->screen.current->count)
        data->screen.selected = 0;
      MenuRender(data->screen.current, &data->screen.selected,
                 &data->objectInfo);
      break;

    case BTN_SEL: {
      menu_item_t *item = &data->screen.current->items[data->screen.selected];

      if (item->type == MENU_ACTION && item->callback) {
        if (data->screen.selected < NUM_PORTS) {
          ShowDataSensorSelection[data->screen.selected].ShowDataScreen = true;
        }

        item->callback(item->ctx);
      } else if (item->type == MENU_SUBMENU && item->children) {
        data->screen.current = item->children;
        data->screen.selected = 0;
        MenuRender(data->screen.current, &data->screen.selected,
                   &data->objectInfo);
      }
      break;
    }

    case BTN_BACK:

      if (data->screen.current->parent) {
        data->screen.current = data->screen.current->parent;
        data->screen.selected = 0;
        if (data->screen.selected < NUM_PORTS) {
          ShowDataSensorSelection[data->screen.selected].ShowDataScreen = false;
        }
        MenuRender(data->screen.current, &data->screen.selected,
                   &data->objectInfo);
      }
      break;

    default:
      break;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
