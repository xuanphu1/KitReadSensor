#ifndef __SENSOR_REGISTRY_H__
#define __SENSOR_REGISTRY_H__

#include "SensorTypes.h"

/* -------------------- Sensor Registry Functions -------------------- */

/**
 * @brief Chuyển đổi SensorType_t enum thành tên string
 * 
 * @param t Loại cảm biến (SensorType_t)
 * @return const char* Tên cảm biến (ví dụ: "BME280", "MH-Z14A")
 */
const char *sensor_type_to_name(SensorType_t t);

/**
 * @brief Lấy mảng sensor drivers đã đăng ký
 * 
 * @return sensor_driver_t* Con trỏ đến mảng sensor_drivers
 * @note Số lượng thực tế được lấy từ sensor_registry_get_count()
 */
sensor_driver_t *sensor_registry_get_drivers(void);

/**
 * @brief Lấy số lượng sensor drivers đã đăng ký
 * 
 * @return size_t Số lượng sensor drivers
 */
size_t sensor_registry_get_count(void);

/**
 * @brief Lấy sensor driver theo loại cảm biến
 * 
 * @param sensor_type Loại cảm biến (SensorType_t)
 * @return sensor_driver_t* Con trỏ đến sensor driver, NULL nếu không tìm thấy
 */
sensor_driver_t *sensor_registry_get_driver(SensorType_t sensor_type);

#endif /* __SENSOR_REGISTRY_H__ */

