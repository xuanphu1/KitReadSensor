#include "SensorRegistry.h"
#include "SensorConfig.h"
#include "esp_log.h"
#include <stddef.h>
#include <stdint.h>

#define TAG_SENSOR_REGISTRY "SENSOR_REGISTRY"

/* -------------------- Test Functions (temporary) -------------------- */
static void TestInit(void) {
  ESP_LOGI(TAG_SENSOR_REGISTRY, "TestInit");
}

static void TestRead(SensorData_t *data) {
  ESP_LOGI(TAG_SENSOR_REGISTRY, "TestRead");
}

/* -------------------- Sensor Drivers Registry -------------------- */
// Mảng chứa thông tin về tất cả các sensor drivers đã đăng ký
static sensor_driver_t sensor_drivers[] = {
    {
        .name = "BME280",
        .init = sensor_bme280_init,
        .read = sensor_bme280_read,
        .deinit = sensor_bme280_deinit,
        .description = {"Temperature", "Pressure", "Humidity"},
        .unit = {"°C", "hPa", "%"},
        .unit_count = 3,
        .is_init = false,
    },
    {
        .name = "MH-Z14A",
        .init = TestInit,
        .read = TestRead,
        .deinit = NULL,
        .description = {"CO2"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "PMS7003",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"PM1.0", "PM2.5", "PM10"},
        .unit = {"ug/m3", "ug/m3", "ug/m3"},
        .unit_count = 3,
        .is_init = false,
    },
    {
        .name = "DHT22",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"Temperature", "Humidity"},
        .unit = {"°C", "%"},
        .unit_count = 2,
        .is_init = false,
    },
    // MQ Series Sensors
    {
        .name = "MQ-2",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"Gas"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "MQ-3",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"Alcohol"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "MQ-4",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"CH4"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "MQ-5",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"LPG"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "MQ-6",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"LPG"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "MQ-7",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"CO"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "MQ-8",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"H2"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "MQ-9",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"CO/LPG"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
    {
        .name = "MQ-135",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"Air Quality"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
    },
};

/* -------------------- Sensor Registry Functions -------------------- */

const char *sensor_type_to_name(SensorType_t t) {
  switch (t) {
  case SENSOR_BME280:
    return "BME280";
  case SENSOR_MHZ14A:
    return "MH-Z14A";
  case SENSOR_PMS7003:
    return "PMS7003";
  case SENSOR_DHT22:
    return "DHT22";
  case SENSOR_MQ2:
    return "MQ-2";
  case SENSOR_MQ3:
    return "MQ-3";
  case SENSOR_MQ4:
    return "MQ-4";
  case SENSOR_MQ5:
    return "MQ-5";
  case SENSOR_MQ6:
    return "MQ-6";
  case SENSOR_MQ7:
    return "MQ-7";
  case SENSOR_MQ8:
    return "MQ-8";
  case SENSOR_MQ9:
    return "MQ-9";
  case SENSOR_MQ135:
    return "MQ-135";
  case SENSOR_NONE:
    return "None";
  default:
    return "Unknown";
  }
}

sensor_driver_t *sensor_registry_get_drivers(void) {
  return sensor_drivers;
}

size_t sensor_registry_get_count(void) {
  return sizeof(sensor_drivers) / sizeof(sensor_drivers[0]);
}

sensor_driver_t *sensor_registry_get_driver(SensorType_t sensor_type) {
  size_t count = sensor_registry_get_count();
  if (sensor_type < 0 || (size_t)sensor_type >= count) {
    return NULL;
  }
  return &sensor_drivers[sensor_type];
}

