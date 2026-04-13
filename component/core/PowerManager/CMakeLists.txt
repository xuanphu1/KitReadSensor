idf_component_register(
    SRCS "PowerManager.c"
    INCLUDE_DIRS "."
    REQUIRES "DataManager" "pcf8574" "PinManager" "AnalogManager"
)
