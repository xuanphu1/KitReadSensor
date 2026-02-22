#ifndef __SENSOR_TYPES_H__
#define __SENSOR_TYPES_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ErrorCodes.h"

/* -------------------- Sensor Definitions -------------------- */
// Số lượng cổng sensor
#define NUM_PORTS 3
// Lưu ý: Số lượng sensor thực tế được lấy từ sensor_registry_get_count()
// Không cần định nghĩa MAX_SENSORS vì sử dụng cấp phát động

/* -------------------- Port ID Enum -------------------- */
// Định danh Port để tracking lựa chọn
typedef enum {
  PORT_NONE = -1,
  PORT_1 = 0,
  PORT_2 = 1,
  PORT_3 = 2,
} PortId_t;

/* -------------------- Sensor Type Enum -------------------- */
// Định danh loại cảm biến
typedef enum {
  SENSOR_NONE = -1,
  SENSOR_BME280 = 0,
  SENSOR_MHZ14A = 1,
  SENSOR_PMS7003 = 2,
  SENSOR_DHT22 = 3,
  // MQ Series Sensors
  SENSOR_MQ2 = 4,
  SENSOR_MQ3 = 5,
  SENSOR_MQ4 = 6,
  SENSOR_MQ5 = 7,
  SENSOR_MQ6 = 8,
  SENSOR_MQ7 = 9,
  SENSOR_MQ8 = 10,
  SENSOR_MQ9 = 11,
  SENSOR_MQ135 = 12,
} SensorType_t;

typedef enum {
  PIN_1 = 0,
  PIN_2 = 1,
  PIN_3 = 2,
  PIN_4 = 3,
  PIN_5 = 4
} PinPort_t;

/** Thứ tự menu: UART, I2C, SPI, ANALOG, PULSE (dùng với sensor_registry_get_count_by_interface) */
typedef enum {
  COMMUNICATION_NONE = -1,
  COMMUNICATION_UART = 0,
  COMMUNICATION_I2C = 1,
  COMMUNICATION_SPI = 2,
  COMMUNICATION_ANALOG = 3,
  COMMUNICATION_PULSE = 4,
  COMMUNICATION_DIGITAL = 5,
} TypeCommunication_t;

/* -------------------- Sensor Data Structure -------------------- */
// Cấu trúc dữ liệu đọc được từ sensor
typedef struct {
  float data_fl[5];
  uint32_t data_uint32[5];
  uint16_t data_uint16[5];
  uint8_t data_uint8[5];
} SensorData_t;

/* -------------------- Sensor Driver Structure -------------------- */
// Cấu trúc cho 1 sensor driver
typedef struct {
  const char *name;                    // Tên sensor (ví dụ: "BME280")
  const char *description[20];         // Mô tả các trường dữ liệu
  const char *unit[20];                // Đơn vị đo
  bool is_init;                        // Trạng thái khởi tạo
  uint8_t unit_count;                  // Số lượng đơn vị/trường dữ liệu
  TypeCommunication_t interface;       // Giao tiếp: UART, I2C, SPI, ANALOG, PULSE
  system_err_t (*init)(void);         // Hàm khởi tạo sensor
  system_err_t (*read)(SensorData_t *); // Hàm đọc dữ liệu vào struct
  system_err_t (*deinit)(void);        // Hàm giải phóng (optional)
} sensor_driver_t;

#endif /* __SENSOR_TYPES_H__ */

