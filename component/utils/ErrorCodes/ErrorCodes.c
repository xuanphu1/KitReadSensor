/**
 * @file ErrorCodes.c
 * @brief Implementation của các helper functions cho ErrorCodes
 */

#include "ErrorCodes.h"

/* ============================================================================
 * ERROR CODE TO STRING MAPPING
 * ============================================================================ */

/**
 * @brief Chuyển đổi error code thành tên string
 */
const char *system_err_to_name(system_err_t err)
{
    // Kiểm tra nếu là ESP-IDF standard error
    if (err < 0x10000) {
        return esp_err_to_name(err);
    }

    // MRS Project custom errors
    switch (err) {
        /* Core Module */
        case MRS_ERR_CORE_INVALID_PARAM:
            return "MRS_ERR_CORE_INVALID_PARAM";
        case MRS_ERR_CORE_NOT_INITIALIZED:
            return "MRS_ERR_CORE_NOT_INITIALIZED";
        case MRS_ERR_CORE_ALREADY_INIT:
            return "MRS_ERR_CORE_ALREADY_INIT";
        case MRS_ERR_CORE_OUT_OF_MEMORY:
            return "MRS_ERR_CORE_OUT_OF_MEMORY";
        case MRS_ERR_CORE_INVALID_STATE:
            return "MRS_ERR_CORE_INVALID_STATE";
        case MRS_ERR_CORE_NOT_FOUND:
            return "MRS_ERR_CORE_NOT_FOUND";
        case MRS_ERR_CORE_TIMEOUT:
            return "MRS_ERR_CORE_TIMEOUT";

        /* DataManager */
        case MRS_ERR_DATAMANAGER_INVALID_PORT:
            return "MRS_ERR_DATAMANAGER_INVALID_PORT";
        case MRS_ERR_DATAMANAGER_PORT_IN_USE:
            return "MRS_ERR_DATAMANAGER_PORT_IN_USE";
        case MRS_ERR_DATAMANAGER_NO_SENSOR:
            return "MRS_ERR_DATAMANAGER_NO_SENSOR";

        /* FunctionManager */
        case MRS_ERR_FUNCTIONMANAGER_TASK_FAILED:
            return "MRS_ERR_FUNCTIONMANAGER_TASK_FAILED";

        /* Sensors Module */
        case MRS_ERR_SENSORS_INVALID_TYPE:
            return "MRS_ERR_SENSORS_INVALID_TYPE";
        case MRS_ERR_SENSORS_NOT_INITIALIZED:
            return "MRS_ERR_SENSORS_NOT_INITIALIZED";
        case MRS_ERR_SENSORS_INIT_FAILED:
            return "MRS_ERR_SENSORS_INIT_FAILED";
        case MRS_ERR_SENSORS_READ_FAILED:
            return "MRS_ERR_SENSORS_READ_FAILED";
        case MRS_ERR_SENSORS_INVALID_DATA:
            return "MRS_ERR_SENSORS_INVALID_DATA";
        case MRS_ERR_SENSORS_NOT_FOUND:
            return "MRS_ERR_SENSORS_NOT_FOUND";
        case MRS_ERR_SENSORS_REGISTRY_FULL:
            return "MRS_ERR_SENSORS_REGISTRY_FULL";

        /* SensorConfig */
        case MRS_ERR_SENSORCONFIG_WRAPPER_FAILED:
            return "MRS_ERR_SENSORCONFIG_WRAPPER_FAILED";

        /* SensorRegistry */
        case MRS_ERR_SENSORREGISTRY_INVALID_INDEX:
            return "MRS_ERR_SENSORREGISTRY_INVALID_INDEX";

        /* BME280 */
        case MRS_ERR_SENSOR_BME280_INIT_FAILED:
            return "MRS_ERR_SENSOR_BME280_INIT_FAILED";
        case MRS_ERR_SENSOR_BME280_READ_FAILED:
            return "MRS_ERR_SENSOR_BME280_READ_FAILED";

        /* UI Module */
        case MRS_ERR_UI_INVALID_MENU:
            return "MRS_ERR_UI_INVALID_MENU";
        case MRS_ERR_UI_INVALID_ITEM:
            return "MRS_ERR_UI_INVALID_ITEM";
        case MRS_ERR_UI_RENDER_FAILED:
            return "MRS_ERR_UI_RENDER_FAILED";
        case MRS_ERR_UI_INVALID_BUTTON:
            return "MRS_ERR_UI_INVALID_BUTTON";

        /* MenuSystem */
        case MRS_ERR_MENUSYSTEM_INVALID_CALLBACK:
            return "MRS_ERR_MENUSYSTEM_INVALID_CALLBACK";
        case MRS_ERR_MENUSYSTEM_NAVIGATION_FAILED:
            return "MRS_ERR_MENUSYSTEM_NAVIGATION_FAILED";

        /* ScreenManager */
        case MRS_ERR_SCREENMANAGER_NOT_INIT:
            return "MRS_ERR_SCREENMANAGER_NOT_INIT";
        case MRS_ERR_SCREENMANAGER_DISPLAY_FAIL:
            return "MRS_ERR_SCREENMANAGER_DISPLAY_FAIL";

        /* ButtonManager */
        case MRS_ERR_BUTTONMANAGER_GPIO_FAILED:
            return "MRS_ERR_BUTTONMANAGER_GPIO_FAILED";

        /* Network Module */
        case MRS_ERR_NETWORK_NOT_INITIALIZED:
            return "MRS_ERR_NETWORK_NOT_INITIALIZED";
        case MRS_ERR_NETWORK_CONNECTION_FAILED:
            return "MRS_ERR_NETWORK_CONNECTION_FAILED";
        case MRS_ERR_NETWORK_TIMEOUT:
            return "MRS_ERR_NETWORK_TIMEOUT";
        case MRS_ERR_NETWORK_INVALID_CONFIG:
            return "MRS_ERR_NETWORK_INVALID_CONFIG";

        /* WifiManager */
        case MRS_ERR_WIFIMANAGER_INIT_FAILED:
            return "MRS_ERR_WIFIMANAGER_INIT_FAILED";
        case MRS_ERR_WIFIMANAGER_STA_FAILED:
            return "MRS_ERR_WIFIMANAGER_STA_FAILED";
        case MRS_ERR_WIFIMANAGER_AP_FAILED:
            return "MRS_ERR_WIFIMANAGER_AP_FAILED";
        case MRS_ERR_WIFIMANAGER_INVALID_SSID:
            return "MRS_ERR_WIFIMANAGER_INVALID_SSID";
        case MRS_ERR_WIFIMANAGER_INVALID_PASS:
            return "MRS_ERR_WIFIMANAGER_INVALID_PASS";

        /* MeshManager */
        case MRS_ERR_MESHMANAGER_INIT_FAILED:
            return "MRS_ERR_MESHMANAGER_INIT_FAILED";
        case MRS_ERR_MESHMANAGER_NOT_SUPPORTED:
            return "MRS_ERR_MESHMANAGER_NOT_SUPPORTED";

        /* Drivers Module */
        case MRS_ERR_DRIVERS_INIT_FAILED:
            return "MRS_ERR_DRIVERS_INIT_FAILED";
        case MRS_ERR_DRIVERS_NOT_INITIALIZED:
            return "MRS_ERR_DRIVERS_NOT_INITIALIZED";
        case MRS_ERR_DRIVERS_INVALID_PARAM:
            return "MRS_ERR_DRIVERS_INVALID_PARAM";
        case MRS_ERR_DRIVERS_COMM_FAILED:
            return "MRS_ERR_DRIVERS_COMM_FAILED";

        /* I2C */
        case MRS_ERR_I2CDEV_INIT_FAILED:
            return "MRS_ERR_I2CDEV_INIT_FAILED";
        case MRS_ERR_I2CDEV_BUSY:
            return "MRS_ERR_I2CDEV_BUSY";
        case MRS_ERR_I2CDEV_NACK:
            return "MRS_ERR_I2CDEV_NACK";
        case MRS_ERR_I2CDEV_TIMEOUT:
            return "MRS_ERR_I2CDEV_TIMEOUT";

        /* SSD1306 */
        case MRS_ERR_SSD1306_INIT_FAILED:
            return "MRS_ERR_SSD1306_INIT_FAILED";
        case MRS_ERR_SSD1306_DISPLAY_FAILED:
            return "MRS_ERR_SSD1306_DISPLAY_FAILED";

        /* DS3231 */
        case MRS_ERR_DS3231_INIT_FAILED:
            return "MRS_ERR_DS3231_INIT_FAILED";
        case MRS_ERR_DS3231_READ_FAILED:
            return "MRS_ERR_DS3231_READ_FAILED";
        case MRS_ERR_DS3231_WRITE_FAILED:
            return "MRS_ERR_DS3231_WRITE_FAILED";

        /* LED RGB */
        case MRS_ERR_LEDRGB_INIT_FAILED:
            return "MRS_ERR_LEDRGB_INIT_FAILED";
        case MRS_ERR_LEDRGB_RMT_FAILED:
            return "MRS_ERR_LEDRGB_RMT_FAILED";

        /* Utils Module */
        case MRS_ERR_UTILS_INVALID_OPERATION:
            return "MRS_ERR_UTILS_INVALID_OPERATION";

        default:
            return "MRS_ERR_UNKNOWN";
    }
}

/**
 * @brief Kiểm tra xem error code có thuộc module cụ thể không
 */
bool system_err_is_module(system_err_t err, uint8_t module_id)
{
    if (err < 0x10000) {
        return false;  // ESP-IDF standard errors không thuộc MRS modules
    }
    
    uint8_t err_module = (err >> 12) & 0xFF;
    return (err_module == module_id);
}

/**
 * @brief Lấy module ID từ error code
 */
uint8_t system_err_get_module(system_err_t err)
{
    if (err < 0x10000) {
        return 0;  // ESP-IDF standard errors không có module ID
    }
    
    return (err >> 12) & 0xFF;
}

/**
 * @brief Lấy error code cụ thể trong module (loại bỏ module ID)
 */
uint16_t system_err_get_code(system_err_t err)
{
    if (err < 0x10000) {
        return err;  // ESP-IDF standard errors
    }
    
    return err & 0xFFF;  // 12-bit error code
}

