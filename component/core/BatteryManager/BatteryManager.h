#ifndef __BATTERY_MANAGER_H__
#define __BATTERY_MANAGER_H__

#include "Common.h"
#include "esp_log.h"
#include "driver/adc.h"
#include <stdint.h>
#include <stdbool.h>

#define TAG_BATTERY_MANAGER "BATTERY_MANAGER"

/**
 * @brief Khởi tạo Battery Manager
 * @return ESP_OK nếu thành công, mã lỗi nếu thất bại
 */
esp_err_t BatteryManager_Init(void);

/**
 * @brief Khởi động FreeRTOS task để đọc pin định kỳ
 * @param data Con trỏ đến DataManager để cập nhật thông tin pin
 */
void BatteryManager_StartTask(DataManager_t *data);

/**
 * @brief Lấy điện áp pin hiện tại (V)
 * @return Điện áp pin tính bằng Volt
 */
float BatteryManager_GetVoltage(void);

/**
 * @brief Lấy dung lượng pin hiện tại (%)
 * @return Dung lượng pin từ 0-100%
 */
uint8_t BatteryManager_GetLevel(void);

/**
 * @brief Lấy index cho hiển thị icon/text
 * @return Index trong mảng (0-6)
 */
uint8_t BatteryManager_GetLevelIndex(void);

/**
 * @brief Cập nhật thông tin pin vào BatteryInfo
 * @param batteryInfo Con trỏ đến BatteryInfo để cập nhật
 */
void BatteryManager_UpdateInfo(BatteryInfo_t *batteryInfo);

#endif /* __BATTERY_MANAGER_H__ */
