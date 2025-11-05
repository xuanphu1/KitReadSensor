#include "DataManager.h"
#include "MenuButton.h"
#include "MenuSystem.h"
#include "ScreenManager.h"
#include "WifiManager.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "ssd1306.h"
#include "nvs_flash.h"
#include "bme280.h"

static const char *TAG_MAIN = "MAIN_PROJECT";
ssd1306_handle_t MainScreen = NULL;
static DataManager_t DataManager; // cấp phát static/global hoặc malloc

// static void I2C_Init(void) {
//   i2c_config_t conf = {.mode = I2C_MODE_MASTER,
//                        .sda_io_num = CONFIG_SDA_GPIO,
//                        .scl_io_num = CONFIG_SCL_GPIO,
//                        .sda_pullup_en = GPIO_PULLUP_ENABLE,
//                        .scl_pullup_en = GPIO_PULLUP_ENABLE,
//                        .master.clk_speed = 400000,
//                        .clk_flags = 0};
//   i2c_param_config(I2C_NUM_0, &conf);
//   i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
//   ESP_LOGI(TAG_MAIN, "I2C init done. SDA=%d SCL=%d", CONFIG_SDA_GPIO,
//     CONFIG_SCL_GPIO);
// }