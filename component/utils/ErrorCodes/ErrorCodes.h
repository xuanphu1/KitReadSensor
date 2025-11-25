/**
 * @file ErrorCodes.h
 * @brief Định nghĩa các mã lỗi chung cho toàn bộ dự án MRS_Project
 * 
 * File này định nghĩa các error codes được sử dụng thống nhất trong toàn bộ dự án.
 * Các error codes được tổ chức theo module và tương thích với esp_err_t của ESP-IDF.
 * 
 * @note Các error codes được định nghĩa theo format:
 *       ((MODULE_ID << 12) | ERROR_CODE)
 *       - MODULE_ID: 8-bit module identifier
 *       - ERROR_CODE: 12-bit error code trong module
 */

#ifndef __ERROR_CODES_H__
#define __ERROR_CODES_H__

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * MODULE IDENTIFIERS
 * ============================================================================ */

/**
 * @brief Module ID cho các component trong dự án
 */
#define MRS_MODULE_CORE           0x01    ///< Core layer (DataManager, FunctionManager)
#define MRS_MODULE_SENSORS         0x02    ///< Sensors layer (SensorTypes, SensorConfig, SensorRegistry)
#define MRS_MODULE_UI              0x03    ///< UI layer (MenuSystem, ScreenManager, ButtonManager)
#define MRS_MODULE_NETWORK         0x04    ///< Network layer (WifiManager, MeshManager)
#define MRS_MODULE_DRIVERS         0x05    ///< Drivers layer (BME280, BMP280, DS3231, SSD1306, LedRGB, i2cdev)
#define MRS_MODULE_UTILS           0x06    ///< Utils layer (BitManager, ErrorCodes)

/* ============================================================================
 * CORE MODULE ERROR CODES (0x01xx)
 * ============================================================================ */

#define MRS_ERR_CORE_BASE          ((MRS_MODULE_CORE << 12) | 0x0000)

#define MRS_ERR_CORE_INVALID_PARAM     (MRS_ERR_CORE_BASE + 0x01)  ///< Invalid parameter
#define MRS_ERR_CORE_NOT_INITIALIZED   (MRS_ERR_CORE_BASE + 0x02)  ///< Component not initialized
#define MRS_ERR_CORE_ALREADY_INIT      (MRS_ERR_CORE_BASE + 0x03)  ///< Component already initialized
#define MRS_ERR_CORE_OUT_OF_MEMORY     (MRS_ERR_CORE_BASE + 0x04)  ///< Out of memory
#define MRS_ERR_CORE_INVALID_STATE     (MRS_ERR_CORE_BASE + 0x05)  ///< Invalid state
#define MRS_ERR_CORE_NOT_FOUND         (MRS_ERR_CORE_BASE + 0x06)  ///< Resource not found
#define MRS_ERR_CORE_TIMEOUT            (MRS_ERR_CORE_BASE + 0x07)  ///< Operation timeout

/* DataManager specific errors */
#define MRS_ERR_DATAMANAGER_BASE       (MRS_ERR_CORE_BASE + 0x10)
#define MRS_ERR_DATAMANAGER_INVALID_PORT   (MRS_ERR_DATAMANAGER_BASE + 0x01)  ///< Invalid port ID
#define MRS_ERR_DATAMANAGER_PORT_IN_USE     (MRS_ERR_DATAMANAGER_BASE + 0x02)  ///< Port already in use
#define MRS_ERR_DATAMANAGER_NO_SENSOR       (MRS_ERR_DATAMANAGER_BASE + 0x03)  ///< No sensor assigned to port

/* FunctionManager specific errors */
#define MRS_ERR_FUNCTIONMANAGER_BASE  (MRS_ERR_CORE_BASE + 0x20)
#define MRS_ERR_FUNCTIONMANAGER_TASK_FAILED (MRS_ERR_FUNCTIONMANAGER_BASE + 0x01)  ///< Failed to create task

/* ============================================================================
 * SENSORS MODULE ERROR CODES (0x02xx)
 * ============================================================================ */

#define MRS_ERR_SENSORS_BASE        ((MRS_MODULE_SENSORS << 12) | 0x0000)

#define MRS_ERR_SENSORS_INVALID_TYPE     (MRS_ERR_SENSORS_BASE + 0x01)  ///< Invalid sensor type
#define MRS_ERR_SENSORS_NOT_INITIALIZED  (MRS_ERR_SENSORS_BASE + 0x02)  ///< Sensor not initialized
#define MRS_ERR_SENSORS_INIT_FAILED      (MRS_ERR_SENSORS_BASE + 0x03)  ///< Sensor initialization failed
#define MRS_ERR_SENSORS_READ_FAILED      (MRS_ERR_SENSORS_BASE + 0x04)  ///< Sensor read failed
#define MRS_ERR_SENSORS_INVALID_DATA     (MRS_ERR_SENSORS_BASE + 0x05)  ///< Invalid sensor data
#define MRS_ERR_SENSORS_NOT_FOUND       (MRS_ERR_SENSORS_BASE + 0x06)  ///< Sensor not found in registry
#define MRS_ERR_SENSORS_REGISTRY_FULL   (MRS_ERR_SENSORS_BASE + 0x07)  ///< Sensor registry full

/* SensorConfig specific errors */
#define MRS_ERR_SENSORCONFIG_BASE   (MRS_ERR_SENSORS_BASE + 0x10)
#define MRS_ERR_SENSORCONFIG_WRAPPER_FAILED (MRS_ERR_SENSORCONFIG_BASE + 0x01)  ///< Wrapper function failed

/* SensorRegistry specific errors */
#define MRS_ERR_SENSORREGISTRY_BASE (MRS_ERR_SENSORS_BASE + 0x20)
#define MRS_ERR_SENSORREGISTRY_INVALID_INDEX (MRS_ERR_SENSORREGISTRY_BASE + 0x01)  ///< Invalid sensor index

/* Individual sensor errors (compatible with existing BME280 errors) */
/* Note: BME280 errors use driver module but are defined separately for compatibility */
/* These are kept for backward compatibility with existing BME280 driver */
#define MRS_ERR_SENSOR_BME280_BASE         ((MRS_MODULE_DRIVERS << 12) | 0x00)  ///< BME280 uses driver module
#define MRS_ERR_SENSOR_BME280_INIT_FAILED  (MRS_ERR_SENSOR_BME280_BASE)  ///< BME280 init failed (0x5000)
#define MRS_ERR_SENSOR_BME280_READ_FAILED  ((MRS_ERR_SENSOR_BME280_BASE) + 0x80)  ///< BME280 read failed (0x5080, moved to avoid conflict)

/* ============================================================================
 * UI MODULE ERROR CODES (0x03xx)
 * ============================================================================ */

#define MRS_ERR_UI_BASE             ((MRS_MODULE_UI << 12) | 0x0000)

#define MRS_ERR_UI_INVALID_MENU         (MRS_ERR_UI_BASE + 0x01)  ///< Invalid menu
#define MRS_ERR_UI_INVALID_ITEM         (MRS_ERR_UI_BASE + 0x02)  ///< Invalid menu item
#define MRS_ERR_UI_RENDER_FAILED        (MRS_ERR_UI_BASE + 0x03)  ///< UI render failed
#define MRS_ERR_UI_INVALID_BUTTON       (MRS_ERR_UI_BASE + 0x04)  ///< Invalid button state

/* MenuSystem specific errors */
#define MRS_ERR_MENUSYSTEM_BASE     (MRS_ERR_UI_BASE + 0x10)
#define MRS_ERR_MENUSYSTEM_INVALID_CALLBACK (MRS_ERR_MENUSYSTEM_BASE + 0x01)  ///< Invalid callback function
#define MRS_ERR_MENUSYSTEM_NAVIGATION_FAILED (MRS_ERR_MENUSYSTEM_BASE + 0x02)  ///< Navigation failed

/* ScreenManager specific errors */
#define MRS_ERR_SCREENMANAGER_BASE  (MRS_ERR_UI_BASE + 0x20)
#define MRS_ERR_SCREENMANAGER_NOT_INIT     (MRS_ERR_SCREENMANAGER_BASE + 0x01)  ///< ScreenManager not initialized
#define MRS_ERR_SCREENMANAGER_DISPLAY_FAIL (MRS_ERR_SCREENMANAGER_BASE + 0x02)  ///< Display operation failed

/* ButtonManager specific errors */
#define MRS_ERR_BUTTONMANAGER_BASE  (MRS_ERR_UI_BASE + 0x30)
#define MRS_ERR_BUTTONMANAGER_GPIO_FAILED  (MRS_ERR_BUTTONMANAGER_BASE + 0x01)  ///< GPIO configuration failed

/* ============================================================================
 * NETWORK MODULE ERROR CODES (0x04xx)
 * ============================================================================ */

#define MRS_ERR_NETWORK_BASE        ((MRS_MODULE_NETWORK << 12) | 0x0000)

#define MRS_ERR_NETWORK_NOT_INITIALIZED   (MRS_ERR_NETWORK_BASE + 0x01)  ///< Network not initialized
#define MRS_ERR_NETWORK_CONNECTION_FAILED (MRS_ERR_NETWORK_BASE + 0x02)  ///< Connection failed
#define MRS_ERR_NETWORK_TIMEOUT           (MRS_ERR_NETWORK_BASE + 0x03)  ///< Network operation timeout
#define MRS_ERR_NETWORK_INVALID_CONFIG    (MRS_ERR_NETWORK_BASE + 0x04)  ///< Invalid network configuration

/* WifiManager specific errors */
#define MRS_ERR_WIFIMANAGER_BASE    (MRS_ERR_NETWORK_BASE + 0x10)
#define MRS_ERR_WIFIMANAGER_INIT_FAILED   (MRS_ERR_WIFIMANAGER_BASE + 0x01)  ///< WiFi initialization failed
#define MRS_ERR_WIFIMANAGER_STA_FAILED    (MRS_ERR_WIFIMANAGER_BASE + 0x02)  ///< STA mode failed
#define MRS_ERR_WIFIMANAGER_AP_FAILED     (MRS_ERR_WIFIMANAGER_BASE + 0x03)  ///< AP mode failed
#define MRS_ERR_WIFIMANAGER_INVALID_SSID  (MRS_ERR_WIFIMANAGER_BASE + 0x04)  ///< Invalid SSID
#define MRS_ERR_WIFIMANAGER_INVALID_PASS  (MRS_ERR_WIFIMANAGER_BASE + 0x05)  ///< Invalid password

/* MeshManager specific errors */
#define MRS_ERR_MESHMANAGER_BASE    (MRS_ERR_NETWORK_BASE + 0x20)
#define MRS_ERR_MESHMANAGER_INIT_FAILED   (MRS_ERR_MESHMANAGER_BASE + 0x01)  ///< Mesh initialization failed
#define MRS_ERR_MESHMANAGER_NOT_SUPPORTED  (MRS_ERR_MESHMANAGER_BASE + 0x02)  ///< Mesh not supported

/* ============================================================================
 * DRIVERS MODULE ERROR CODES (0x05xx)
 * ============================================================================ */

#define MRS_ERR_DRIVERS_BASE        ((MRS_MODULE_DRIVERS << 12) | 0x0000)

#define MRS_ERR_DRIVERS_INIT_FAILED      (MRS_ERR_DRIVERS_BASE + 0x01)  ///< Driver initialization failed
#define MRS_ERR_DRIVERS_NOT_INITIALIZED (MRS_ERR_DRIVERS_BASE + 0x02)  ///< Driver not initialized
#define MRS_ERR_DRIVERS_INVALID_PARAM   (MRS_ERR_DRIVERS_BASE + 0x03)  ///< Invalid driver parameter
#define MRS_ERR_DRIVERS_COMM_FAILED      (MRS_ERR_DRIVERS_BASE + 0x04)  ///< Communication failed

/* I2C driver errors */
#define MRS_ERR_I2CDEV_BASE         (MRS_ERR_DRIVERS_BASE + 0x10)
#define MRS_ERR_I2CDEV_INIT_FAILED      (MRS_ERR_I2CDEV_BASE + 0x01)  ///< I2C initialization failed
#define MRS_ERR_I2CDEV_BUSY             (MRS_ERR_I2CDEV_BASE + 0x02)  ///< I2C bus busy
#define MRS_ERR_I2CDEV_NACK              (MRS_ERR_I2CDEV_BASE + 0x03)  ///< I2C NACK received
#define MRS_ERR_I2CDEV_TIMEOUT           (MRS_ERR_I2CDEV_BASE + 0x04)  ///< I2C timeout

/* SSD1306 display errors */
#define MRS_ERR_SSD1306_BASE        (MRS_ERR_DRIVERS_BASE + 0x20)
#define MRS_ERR_SSD1306_INIT_FAILED     (MRS_ERR_SSD1306_BASE + 0x01)  ///< SSD1306 initialization failed
#define MRS_ERR_SSD1306_DISPLAY_FAILED  (MRS_ERR_SSD1306_BASE + 0x02)  ///< Display operation failed

/* DS3231 RTC errors */
#define MRS_ERR_DS3231_BASE         (MRS_ERR_DRIVERS_BASE + 0x30)
#define MRS_ERR_DS3231_INIT_FAILED      (MRS_ERR_DS3231_BASE + 0x01)  ///< DS3231 initialization failed
#define MRS_ERR_DS3231_READ_FAILED      (MRS_ERR_DS3231_BASE + 0x02)  ///< DS3231 read failed
#define MRS_ERR_DS3231_WRITE_FAILED     (MRS_ERR_DS3231_BASE + 0x03)  ///< DS3231 write failed

/* LED RGB errors */
#define MRS_ERR_LEDRGB_BASE         (MRS_ERR_DRIVERS_BASE + 0x40)
#define MRS_ERR_LEDRGB_INIT_FAILED      (MRS_ERR_LEDRGB_BASE + 0x01)  ///< LED RGB initialization failed
#define MRS_ERR_LEDRGB_RMT_FAILED       (MRS_ERR_LEDRGB_BASE + 0x02)  ///< RMT peripheral failed

/* ============================================================================
 * UTILS MODULE ERROR CODES (0x06xx)
 * ============================================================================ */

#define MRS_ERR_UTILS_BASE          ((MRS_MODULE_UTILS << 12) | 0x0000)

#define MRS_ERR_UTILS_INVALID_OPERATION  (MRS_ERR_UTILS_BASE + 0x01)  ///< Invalid operation

/* ============================================================================
 * ERROR RETURN TYPE
 * ============================================================================ */

/**
 * @brief Kiểu dữ liệu trả về lỗi cho các hàm trong MRS Project
 * 
 * Kiểu này tương thích với esp_err_t của ESP-IDF và có thể sử dụng
 * thay thế cho esp_err_t trong toàn bộ dự án.
 * 
 * @note Sử dụng system_err_t thay vì esp_err_t để có thể mở rộng sau này
 */
typedef esp_err_t system_err_t;

/**
 * @brief Macro định nghĩa thành công
 */
#define MRS_OK    ESP_OK

/**
 * @brief Macro định nghĩa thất bại (generic)
 */
#define MRS_FAIL  ESP_FAIL

/* ============================================================================
 * HELPER MACROS
 * ============================================================================ */

/**
 * @brief Kiểm tra xem kết quả có thành công không
 * 
 * @param err Error code để kiểm tra
 * @return true Nếu thành công (ESP_OK)
 * @return false Nếu có lỗi
 */
#define MRS_IS_OK(err)          ((err) == ESP_OK)

/**
 * @brief Kiểm tra xem kết quả có lỗi không
 * 
 * @param err Error code để kiểm tra
 * @return true Nếu có lỗi
 * @return false Nếu thành công
 */
#define MRS_IS_ERROR(err)        ((err) != ESP_OK)

/**
 * @brief Kiểm tra xem error code có thuộc module cụ thể không (macro version)
 * 
 * @param err Error code
 * @param module_id Module ID (MRS_MODULE_*)
 * @return true Nếu error code thuộc module
 * @return false Nếu không thuộc module
 */
#define MRS_IS_MODULE(err, module_id) \
    (((err) >= 0x10000) && (((err) >> 12) & 0xFF) == (module_id))

/**
 * @brief Kiểm tra xem error code có phải là MRS custom error không
 * 
 * @param err Error code
 * @return true Nếu là MRS custom error
 * @return false Nếu là ESP-IDF standard error
 */
#define MRS_IS_CUSTOM_ERROR(err) ((err) >= 0x10000)

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

/**
 * @brief Chuyển đổi error code thành tên string
 * 
 * @param err Error code (system_err_t hoặc esp_err_t)
 * @return const char* Tên error code hoặc "Unknown error" nếu không tìm thấy
 */
const char *system_err_to_name(system_err_t err);

/**
 * @brief Kiểm tra xem error code có thuộc module cụ thể không (function version)
 * 
 * @param err Error code
 * @param module_id Module ID (MRS_MODULE_*)
 * @return true Nếu error code thuộc module
 * @return false Nếu không thuộc module
 * 
 * @note Sử dụng macro MRS_IS_MODULE() cho hiệu suất tốt hơn
 */
bool system_err_is_module(system_err_t err, uint8_t module_id);

/**
 * @brief Lấy module ID từ error code
 * 
 * @param err Error code
 * @return uint8_t Module ID hoặc 0 nếu không hợp lệ
 */
uint8_t system_err_get_module(system_err_t err);

/**
 * @brief Lấy error code cụ thể trong module (loại bỏ module ID)
 * 
 * @param err Error code
 * @return uint16_t Error code trong module (12-bit)
 */
uint16_t system_err_get_code(system_err_t err);

#ifdef __cplusplus
}
#endif

#endif /* __ERROR_CODES_H__ */

