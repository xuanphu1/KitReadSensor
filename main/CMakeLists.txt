idf_component_register(SRCS "MRS_Project.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash WifiManager DataManager MenuButton MenuSystem BME280
                    )
