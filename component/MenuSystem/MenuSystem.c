#include "MenuSystem.h"
#include "DataManager.h"
#include <sys/_types.h>
#include <time.h>



menu_list_t WiFi_Config_Menu;
DataManager_t Data = {.MenuReturn[0] = &WiFi_Config_Menu};



/* -------------------- Menu Tree -------------------- */
// Submenu "Read Sensor"
menu_item_t Sensor_Port_1_Items[] = {
    {"BME280", MENU_ACTION, read_temperature_cb, NULL, NULL},
    {"MH-Z14A", MENU_ACTION, read_humidity_cb, NULL, NULL},
    {"PMS7003", MENU_ACTION, read_pressure_cb, NULL, NULL},
    {"DHT22", MENU_ACTION, read_dht22_cb, NULL, NULL},
};
menu_item_t Sensor_Port_2_Items[] = {
    {"BME280", MENU_ACTION, read_temperature_cb, NULL, NULL},
    {"MH-Z14A", MENU_ACTION, read_humidity_cb, NULL, NULL},
    {"PMS7003", MENU_ACTION, read_pressure_cb, NULL, NULL},
    {"DHT22", MENU_ACTION, read_dht22_cb, NULL, NULL},
};
menu_item_t Sensor_Port_3_Items[] = {
    {"BME280", MENU_ACTION, read_temperature_cb, NULL, NULL},
    {"MH-Z14A", MENU_ACTION, read_humidity_cb, NULL, NULL},
    {"PMS7003", MENU_ACTION, read_pressure_cb, NULL, NULL},
    {"DHT22", MENU_ACTION, read_dht22_cb, NULL, NULL},
};

menu_list_t Sensor_Port_1 = {
    .items = Sensor_Port_1_Items,
    .count = ARRAY_SIZE(Sensor_Port_1_Items),
    .parent = NULL,
};
menu_list_t Sensor_Port_2 = {
    .items = Sensor_Port_2_Items,
    .count = ARRAY_SIZE(Sensor_Port_2_Items),
    .parent = NULL,
};
menu_list_t Sensor_Port_3 = {
    .items = Sensor_Port_3_Items,
    .count = ARRAY_SIZE(Sensor_Port_3_Items),
    .parent = NULL,
};
menu_item_t SensorPort[] = {
  {"Port 1", MENU_SUBMENU, NULL, NULL, &Sensor_Port_1},
  {"Port 2", MENU_SUBMENU, NULL, NULL, &Sensor_Port_2},
  {"Port 3", MENU_SUBMENU, NULL, NULL, &Sensor_Port_3},
  // {"Start Read Sensor", MENU_ACTION, NULL, NULL, NULL}
  
};

menu_list_t Sensor_Menu = {
    .items = SensorPort,
    .count = ARRAY_SIZE(SensorPort),
    .parent = NULL, // gán sau
};

// Submenu "WiFi Config"
menu_item_t WiFi_Config_Items[] = {
    {"OK", MENU_ACTION, wifi_config_callback, &Data, NULL},
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
  Sensor_Port_1.parent = &Sensor_Menu;
  Sensor_Port_2.parent = &Sensor_Menu;
  Sensor_Port_3.parent = &Sensor_Menu;
}

/* -------------------- Menu Functions -------------------- */

void MenuSystemInit() {
  Data.screen.current = &Root_Menu;
  Data.screen.selected = 0;
  Data.screen.prev_selected = 0;
  Data.MenuReturn[0] = &WiFi_Config_Menu;
}

/* -------------------- Navigation Task -------------------- */
void NavigationScreen_Task(void *pvParameter) {
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
      MenuRender(data->screen.current, &data->screen.selected, &data->objectInfo);
      break;

    case BTN_DOWN:
      data->screen.prev_selected = data->screen.selected;
      data->screen.selected++;
      if (data->screen.selected >= data->screen.current->count)
        data->screen.selected = 0;
      MenuRender(data->screen.current, &data->screen.selected, &data->objectInfo);
      break;

    case BTN_SEL: {
      menu_item_t *item = &data->screen.current->items[data->screen.selected];

      if (item->type == MENU_ACTION && item->callback) {
        item->callback(item->ctx);
      } else if (item->type == MENU_SUBMENU && item->children) {
        data->screen.current = item->children;
        data->screen.selected = 0;
        MenuRender(data->screen.current, &data->screen.selected, &data->objectInfo);
      }
      break;
    }

    case BTN_BACK:

      if (data->screen.current->parent) {
        data->screen.current = data->screen.current->parent;
        data->screen.selected = 0;

        MenuRender(data->screen.current, &data->screen.selected, &data->objectInfo);
      }
      break;

    default:
      break;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
