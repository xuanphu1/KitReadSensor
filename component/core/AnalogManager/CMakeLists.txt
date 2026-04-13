idf_component_register(
    SRCS "AnalogManager.c"
    INCLUDE_DIRS "."
    REQUIRES "i2cdev" "PinManager" "SensorTypes" "ErrorCodes"
)
