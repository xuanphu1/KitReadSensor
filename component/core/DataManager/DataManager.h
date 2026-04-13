#ifndef __DATA_MANAGER_H__
#define __DATA_MANAGER_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "SensorTypes.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MAX_TEXT_LENGTH 21
#define NUM_OBJECT_MAX 5
#define NUM_SENSORS 20
#define BTN_UP_GPIO 18 //13
#define BTN_DOWN_GPIO 14 //14
#define BTN_SEL_GPIO 25 //25
#define BTN_BACK_GPIO 13 //35
#define MAX_VISIBLE_ITEMS 4
/* -------------------- Button -------------------- */
typedef enum { BTN_UP, BTN_DOWN, BTN_SEL, BTN_BACK, BTN_NONE } button_type_t;

typedef enum {
  IMAGE_BATTERY_0,
  IMAGE_BATTERY_17,
  IMAGE_BATTERY_33,
  IMAGE_BATTERY_50,
  IMAGE_BATTERY_67,
  IMAGE_BATTERY_83,
  IMAGE_BATTERY_FULL,
} Image_Battery_t;

typedef enum {
  IMAGE_WIFI_NOT_CONNECTED,
  IMAGE_WIFI_CONNECTED,
} Image_Wifi_t;

typedef enum {
  DISCONNECTED,
  CONNECTED,
  ERROR,
} status;

typedef enum {
  OBJECT_WIFI,
  OBJECT_BATTERY,
  OBJECT_DIFFERENT,
  OBJECT_SENSOR,
  OBJECT_INFORMATION,
  OBJECT_WIFI_MESH,
  OBJECT_NONE,
} object_type_t;

enum {
  TASK_MESH_UDP_CLIENT,
  TASK_WIFI_CONFIG,
  TASK_WIFI_MESH_JOIN,
  TASK_READ_SENSOR,
  TASK_SHOW_DATA_SENSOR,
  TASK_BATTERY_STATUS,
  TASK_RESET_ALL_PORTS,
  TASK_ACTUATOR_ON,
  TASK_ACTUATOR_OFF,
  TASK_NONE,
};


typedef struct {
  uint8_t state;
  uint32_t last_press_ms; // debounce support
} Button_t;

typedef struct {
  Button_t up;
  Button_t down;
  Button_t sel;
  Button_t back;
} ButtonManager_t;

/* -------------------- Menu -------------------- */
typedef enum {
  MENU_ACTION,
  MENU_SUBMENU,
  MENU_READ_SENSOR,
  MENU_NONE
} menu_item_type_t;

struct menu_list; // forward declare

typedef struct {
  const unsigned char ***image;
  const uint8_t width;
  const uint8_t height;
} image_t;

typedef struct {
  const char ***text;
  const uint8_t size;
} text_t;

typedef struct menu_item {
  const char *name;
  menu_item_type_t type;
  void (*callback)(void *ctx); // callback + user context
  void *ctx;                   // opaque argument for callback
  struct menu_list *children;  // if type == MENU_SUBMENU
} menu_item_t;

typedef struct menu_list {
  menu_item_t *items;
  text_t text;
  image_t image;
  size_t count;
  object_type_t object;
  struct menu_list *parent; // back navigation
  int8_t port_index;        // 0..NUM_PORTS-1: one Port row; >=NUM_PORTS: all ports (e.g. Actuators); -1: none
} menu_list_t;

/* -------------------- Screen -------------------- */
typedef struct {
  menu_list_t *current;
  int8_t selected;
  int8_t prev_selected;
} ScreenManager_t;

/* -------------------- Battery & WiFi Info -------------------- */
typedef struct {
  uint8_t batteryLevel;
  char *batteryName;
} BatteryInfo_t;

typedef struct {
  status wifiStatus;
  char *wifiName;
} wifiInfo_t;

typedef struct {
  status meshStatus;
  char *ipRoot;
} meshInfo_t;

typedef struct {
  BatteryInfo_t batteryInfo;
  wifiInfo_t wifiInfo;
  meshInfo_t meshInfo;
  /** Per-port selected sensor name for Port 1/2/3 rows (MenuSystem fills from selectedSensor) */
  const char *selectedSensorName[NUM_PORTS];
} objectInfoManager_t;

/* -------------------- Data Manager (App Context) -------------------- */
// PortId_t and SensorType_t are defined in SensorTypes.h

struct DataManager_t;
/** After sensor pick: refresh Port labels and return to Sensors (set by MenuSystem). */
typedef void (*on_sensor_selected_fn)(struct DataManager_t *data, int port);
/** After reset all ports: restore default Port labels (set by MenuSystem). */
typedef void (*on_ports_reset_fn)(struct DataManager_t *data);

typedef struct DataManager_t {
  SensorData_t sensor;
  ButtonManager_t button;
  ScreenManager_t screen;
  objectInfoManager_t objectInfo;
  menu_list_t *MenuReturn[10];
  SensorType_t selectedSensor[NUM_PORTS];
  on_sensor_selected_fn on_sensor_selected;  // optional, set by MenuSystem
  on_ports_reset_fn on_ports_reset;           // optional, set by MenuSystem

  TaskHandle_t TaskHandle_Array[10];

  uint8_t version[3];
} DataManager_t;

// Argument bundle for sensor selection callback
typedef struct {
  DataManager_t *data;
  PortId_t port;
  SensorType_t sensor;
} SelectionParam_t;


typedef struct {
  DataManager_t *data;
  PortId_t port;
  bool ShowDataScreen;
} ShowDataSensorParam_t;

#endif /* __DATA_MANAGER_H__ */

