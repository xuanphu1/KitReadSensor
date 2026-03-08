idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash WifiManager DataManager ButtonManager MenuSystem BME280 LedRGB BatteryManager DS3231 SD_Card
                    )
