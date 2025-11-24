#ifndef __LED_RGB_H__
#define __LED_RGB_H__

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Khởi tạo LED RGB
 * 
 * @return esp_err_t ESP_OK nếu thành công
 */
esp_err_t LedRGB_Init(void);

/**
 * @brief Đặt màu LED RGB
 * 
 * @param red Giá trị màu đỏ (0-255)
 * @param green Giá trị màu xanh lá (0-255)
 * @param blue Giá trị màu xanh dương (0-255)
 * @return esp_err_t ESP_OK nếu thành công
 */
esp_err_t LedRGB_SetColor(uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Tắt LED RGB
 * 
 * @return esp_err_t ESP_OK nếu thành công
 */
esp_err_t LedRGB_Off(void);

/**
 * @brief Sáng LED với màu tương ứng với nút được ấn
 * 
 * @param button_type Loại nút (BTN_UP, BTN_DOWN, BTN_SEL, BTN_BACK)
 */
void LedRGB_SetButtonColor(int button_type);

#endif /* __LED_RGB_H__ */

