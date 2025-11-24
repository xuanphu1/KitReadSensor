#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "SensorTypes.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MAX_TEXT_LENGTH 21
#define NUM_OBJECT_MAX 5
#define NUM_SENSORS 20
#define BTN_UP_GPIO 18
#define BTN_DOWN_GPIO 19
#define BTN_SEL_GPIO 20
#define BTN_BACK_GPIO 21
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
  OBJECT_NONE,
} object_type_t;

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
  BatteryInfo_t batteryInfo;
  wifiInfo_t wifiInfo;
} objectInfoManager_t;

/* -------------------- Data Manager (App Context) -------------------- */
// Note: PortId_t và SensorType_t đã được định nghĩa trong SensorTypes.h

typedef struct {
  SensorData_t sensor;
  ButtonManager_t button;
  ScreenManager_t screen;
  objectInfoManager_t objectInfo;
  menu_list_t *MenuReturn[10];
  // Mapping lựa chọn: mỗi port -> loại cảm biến đang chọn
  SensorType_t selectedSensor[NUM_PORTS]; // dùng SENSOR_*; -1 nếu chưa chọn

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

