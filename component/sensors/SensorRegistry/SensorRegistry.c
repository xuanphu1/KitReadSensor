#include "SensorRegistry.h"
#include "SensorConfig.h"
#include "esp_log.h"
#include <stddef.h>
#include <stdint.h>

#define TAG_SENSOR_REGISTRY "SENSOR_REGISTRY"

/* -------------------- Test Functions (temporary) -------------------- */
static system_err_t TestInit(void) {
  ESP_LOGI(TAG_SENSOR_REGISTRY, "TestInit");
  return MRS_OK;
}

static system_err_t TestRead(SensorData_t *data) {
  if (data == NULL) {
    ESP_LOGW(TAG_SENSOR_REGISTRY, "TestRead: data is NULL");
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  ESP_LOGI(TAG_SENSOR_REGISTRY, "TestRead");
  return MRS_OK;
}

/* -------------------- Sensor Drivers Registry -------------------- */
// Mảng chứa thông tin về tất cả các sensor drivers đã đăng ký
static sensor_driver_t sensor_drivers[] = {
    {
        .name = "BME280",
        .init = bme280Initialize,
        .read = bme280ReadData,
        .deinit = bme280Deinitialize,
        .description = {"Temperature", "Pressure", "Humidity"},
        .unit = {"°C", "hPa", "%"},
        .unit_count = 3,
        .is_init = false,
        .interface = COMMUNICATION_I2C,
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
        .interface = COMMUNICATION_UART,
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
        .interface = COMMUNICATION_UART,
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
        .interface = COMMUNICATION_PULSE,
    },
    // MQ Series Sensors (analog)
    {
        .name = "MQ-2",
        .init = NULL,
        .read = NULL,
        .deinit = NULL,
        .description = {"Gas"},
        .unit = {"ppm"},
        .unit_count = 1,
        .is_init = false,
        .interface = COMMUNICATION_ANALOG,
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
        .interface = COMMUNICATION_ANALOG,
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
        .interface = COMMUNICATION_ANALOG,
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
        .interface = COMMUNICATION_ANALOG,
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
        .interface = COMMUNICATION_ANALOG,
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
        .interface = COMMUNICATION_ANALOG,
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
        .interface = COMMUNICATION_ANALOG,
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
        .interface = COMMUNICATION_ANALOG,
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
        .interface = COMMUNICATION_ANALOG,
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

size_t sensor_registry_get_count_by_interface(TypeCommunication_t iface) {
  size_t total = sensor_registry_get_count();
  size_t n = 0;
  for (size_t i = 0; i < total; i++) {
    if (sensor_drivers[i].interface == iface) {
      n++;
    }
  }
  return n;
}

sensor_driver_t *sensor_registry_get_driver_at_interface(TypeCommunication_t iface,
                                                         size_t index,
                                                         SensorType_t *out_sensor_type) {
  size_t total = sensor_registry_get_count();
  size_t k = 0;
  for (size_t i = 0; i < total; i++) {
    if (sensor_drivers[i].interface != iface) {
      continue;
    }
    if (k == index) {
      if (out_sensor_type) {
        *out_sensor_type = (SensorType_t)i;
      }
      return &sensor_drivers[i];
    }
    k++;
  }
  return NULL;
}

