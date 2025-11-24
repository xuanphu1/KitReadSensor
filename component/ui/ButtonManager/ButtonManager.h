#ifndef __BUTTON_MANAGER_H__
#define __BUTTON_MANAGER_H__

#include "stdint.h"
#include "Common.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1306.h"

#define TAG_BUTTON_MANAGER "BUTTON_MANAGER"

void ButtonManagerInit(void);
// void NavigationPointer(ssd1306_handle_t oled, uint8_t value, uint8_t prev_value);
button_type_t ReadButtonStatus(void);


#endif

