#ifndef __COMMON_H__
#define __COMMON_H__

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
#define BTN_DOWN_GPIO 19 //14
#define BTN_SEL_GPIO 20 //25
#define BTN_BACK_GPIO 21 //35
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
  uint32_t last_press_ms; // hỗ trợ debounce
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
  void (*callback)(void *ctx); // callback kèm context
  void *ctx;                   // dữ liệu truyền vào callback
  struct menu_list *children;  // nếu type == MENU_SUBMENU
} menu_item_t;

typedef struct menu_list {
  menu_item_t *items;
  text_t text;
  image_t image;
  size_t count;
  object_type_t object;
  struct menu_list *parent; // để quay lại menu cha
  int8_t port_index;        // 0..NUM_PORTS-1 = hiển thị 1 dòng Port đã chọn; >= NUM_PORTS = hiển thị cả các port (vd Actuators); -1 = không
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
  /** Tên cảm biến đã chọn theo port (để hiển thị ở menu Port 1/2/3), do MenuSystem cập nhật từ selectedSensor */
  const char *selectedSensorName[NUM_PORTS];
} objectInfoManager_t;

/* -------------------- Data Manager (App Context) -------------------- */
// Note: PortId_t và SensorType_t đã được định nghĩa trong SensorTypes.h

struct DataManager_t;
/** Callback sau khi chọn cảm biến: cập nhật tên menu Port và quay về menu Sensors (MenuSystem gán). */
typedef void (*on_sensor_selected_fn)(struct DataManager_t *data, int port);
/** Callback sau khi reset all ports: cập nhật tên về "Port 1", "Port 2", "Port 3" (MenuSystem gán). */
typedef void (*on_ports_reset_fn)(struct DataManager_t *data);

typedef struct DataManager_t {
  SensorData_t sensor;
  ButtonManager_t button;
  ScreenManager_t screen;
  objectInfoManager_t objectInfo;
  menu_list_t *MenuReturn[10];
  SensorType_t selectedSensor[NUM_PORTS];
  on_sensor_selected_fn on_sensor_selected;  // optional, MenuSystem gán
  on_ports_reset_fn on_ports_reset;           // optional, MenuSystem gán

  TaskHandle_t TaskHandle_Array[10];
} DataManager_t;

// Tham số truyền qua callback chọn cảm biến
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

#endif /* __COMMON_H__ */

