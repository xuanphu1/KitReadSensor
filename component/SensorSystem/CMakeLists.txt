idf_component_register(
    SRCS "SensorSystem.c"
    INCLUDE_DIRS "."
    REQUIRES "DataManager" "BME280"
)
