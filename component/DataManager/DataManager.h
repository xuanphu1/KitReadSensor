#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define MAX_TEXT_LENGTH 21
#define NUM_OBJECT_MAX 5
// Số lượng cổng sensor
#define NUM_PORTS 3
// Số lượng loại cảm biến thực tế (không bao gồm SENSOR_NONE)
#define NUM_ACTIVE_SENSORS 4
#define NUM_SENSORS 20
#define BTN_UP_GPIO 18
#define BTN_DOWN_GPIO 19
#define BTN_SEL_GPIO 20
#define BTN_BACK_GPIO 21

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

/* -------------------- Sensor -------------------- */
typedef struct {
  float data_fl[5];
  uint32_t data_uint32[5];
  uint16_t data_uint16[5];
  uint8_t data_uint8[5];
} SensorData_t;

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

/* Cấu trúc cho 1 sensor driver */
typedef struct {
  const char *name;
  const char *description[20];
  const char *unit[20];
  bool is_init;
  uint8_t unit_count;           // số lượng đơn vị
  void (*init)(void);           // khởi tạo sensor
  void (*read)(SensorData_t *); // đọc dữ liệu vào struct
  void (*deinit)(void);         // optional, nếu cần
} sensor_driver_t;

/* -------------------- Data Manager (App Context) -------------------- */
// Định danh Port và loại cảm biến để tracking lựa chọn
typedef enum {
  PORT_NONE = -1,
  PORT_1 = 0,
  PORT_2 = 1,
  PORT_3 = 2,
} PortId_t;

typedef enum {
  SENSOR_NONE = -1,
  SENSOR_BME280 = 0,
  SENSOR_MHZ14A = 1,
  SENSOR_PMS7003 = 2,
  SENSOR_DHT22 = 3,
} SensorType_t;

typedef struct {
  SensorData_t sensor;
  ButtonManager_t button;
  ScreenManager_t screen;
  objectInfoManager_t objectInfo;
  menu_list_t *MenuReturn[10];
  // Mapping lựa chọn: mỗi port -> loại cảm biến đang chọn
  int8_t selectedSensor[NUM_PORTS]; // dùng SENSOR_*; -1 nếu chưa chọn

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
} ShowDataSensorParam_t;

#endif /* __SYMBOL_H__ */
