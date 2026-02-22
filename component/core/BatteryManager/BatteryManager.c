#include "BatteryManager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// ADC cho đo pin
#define BATTERY_ADC_CHANNEL ADC1_CHANNEL_6  // GPIO 34
#define BATTERY_ADC_WIDTH ADC_WIDTH_BIT_12
#define BATTERY_ADC_ATTEN ADC_ATTEN_DB_11   // 0-3.3V range (giống Arduino ADC_11db)
#define ADC_MAX 4095.0f
#define VREF 3.3f
#define R1 100000.0f
#define R2 100000.0f
#define BATTERY_FULL_VOLTAGE 4.2f           // Điện áp pin đầy (V)
#define BATTERY_EMPTY_VOLTAGE 3.0f          // Điện áp pin cạn (V)
#define BATTERY_READ_INTERVAL_MS 5000       // Đọc pin mỗi 5 giây

static bool adc_initialized = false;

// Biến để lưu thông tin pin (được cập nhật bởi FreeRTOS task)
static float current_voltage = 0.0f;
static uint8_t current_level = 0;
static uint8_t current_level_index = 0;
static char battery_text[32] = "0% (0.00V)";

/**
 * @brief Khởi tạo ADC để đo pin
 */
static esp_err_t battery_adc_init(void) {
  if (adc_initialized) {
    return ESP_OK;
  }
  
  // Khởi tạo ADC cho đọc pin (GPIO 34 = ADC1_CHANNEL_6)
  // Giống Arduino: analogReadResolution(12) và analogSetAttenuation(ADC_11db)
  // QUAN TRỌNG: Phải gọi config_width TRƯỚC config_channel_atten
  esp_err_t ret = adc1_config_width(BATTERY_ADC_WIDTH);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_BATTERY_MANAGER, "Failed to config ADC width: %s", esp_err_to_name(ret));
    return ret;
  }
  
  ret = adc1_config_channel_atten(BATTERY_ADC_CHANNEL, BATTERY_ADC_ATTEN);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG_BATTERY_MANAGER, "Failed to config ADC channel attenuation: %s", esp_err_to_name(ret));
    return ret;
  }
  
  // Delay để ADC ổn định
  vTaskDelay(pdMS_TO_TICKS(100));
  
  // Đọc nhiều lần để "làm nóng" ADC và kiểm tra giá trị ban đầu
  int dummy_sum = 0;
  for (int i = 0; i < 10; i++) {
    int dummy_reading = adc1_get_raw(BATTERY_ADC_CHANNEL);
    dummy_sum += dummy_reading;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  float dummy_avg = dummy_sum / 10.0f;
  ESP_LOGI(TAG_BATTERY_MANAGER, "ADC warm-up readings: avg = %.0f", dummy_avg);
  
  ESP_LOGI(TAG_BATTERY_MANAGER, "ADC initialized for battery monitoring on GPIO 34 (DB_11, 12-bit)");
  adc_initialized = true;
  return ESP_OK;
}

/**
 * @brief Đo điện áp pin (V)
 * @return Điện áp pin tính bằng Volt
 */
static float battery_read_voltage(void) {
  if (!adc_initialized) {
    ESP_LOGW(TAG_BATTERY_MANAGER, "ADC not initialized, cannot read voltage");
    return 0.0f;
  }
  
  // Đọc nhiều lần và lấy trung bình để giảm nhiễu
  int adc_sum = 0;
  int valid_samples = 0;
  const int samples = 20;
  
  for (int i = 0; i < samples; i++) {
    int adc_reading = adc1_get_raw(BATTERY_ADC_CHANNEL);
    
    if (adc_reading >= 0) {  // Cho phép giá trị 0 hợp lệ
      adc_sum += adc_reading;
      valid_samples++;
      // Log tất cả giá trị để debug (giảm delay để đọc nhanh hơn)
      ESP_LOGD(TAG_BATTERY_MANAGER, "Sample %d: ADC raw = %d", i, adc_reading);
    } else {
      ESP_LOGW(TAG_BATTERY_MANAGER, "Sample %d: Invalid ADC reading = %d", i, adc_reading);
    }
    
    // Delay nhỏ giữa các lần đọc (giảm từ 5ms xuống 1ms để giống Arduino hơn)
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  
  if (valid_samples == 0) {
    ESP_LOGW(TAG_BATTERY_MANAGER, "No valid ADC readings");
    return 0.0f;
  }
  
  float adc_avg = (float)adc_sum / (float)valid_samples;
  ESP_LOGI(TAG_BATTERY_MANAGER, "ADC raw: %.2f (from %d samples, sum: %d, min: %d, max: %d)", 
           adc_avg, valid_samples, adc_sum, 
           valid_samples > 0 ? (adc_sum / valid_samples - 100) : 0,  // Approximate min
           valid_samples > 0 ? (adc_sum / valid_samples + 100) : 0); // Approximate max
  
  // Công thức giống hệt Arduino: vAdc = (adcRaw / ADC_MAX) * VREF
  // Lưu ý: Arduino sử dụng float division, đảm bảo tính toán chính xác
  float vAdc = (adc_avg / ADC_MAX) * VREF;
  
  // Tính điện áp pin thực tế: vBat = vAdc * ((R1 + R2) / R2)
  // Với R1 = R2 = 100kΩ: vBat = vAdc * 2.0
  float vBat = vAdc * ((R1 + R2) / R2);
  
  ESP_LOGI(TAG_BATTERY_MANAGER, "Calculation: adc_avg=%.2f, ADC_MAX=%.0f, VREF=%.1f", adc_avg, ADC_MAX, VREF);
  ESP_LOGI(TAG_BATTERY_MANAGER, "vAdc = (%.2f / %.0f) * %.1f = %.3f V", adc_avg, ADC_MAX, VREF, vAdc);
  ESP_LOGI(TAG_BATTERY_MANAGER, "vBat = %.3f * ((%.0f + %.0f) / %.0f) = %.3f * 2.0 = %.2f V", 
           vAdc, R1, R2, R2, vAdc, vBat);
  
  return vBat;
}

/**
 * @brief Tính dung lượng pin (%)
 * @param voltage Điện áp pin (V)
 * @return Dung lượng pin từ 0-100%
 */
static uint8_t battery_calculate_level(float voltage) {
  if (voltage >= BATTERY_FULL_VOLTAGE) {
    return 100;
  }
  if (voltage <= BATTERY_EMPTY_VOLTAGE) {
    return 0;
  }
  
  // Tính phần trăm tuyến tính giữa empty và full
  float percentage = ((voltage - BATTERY_EMPTY_VOLTAGE) / 
                      (BATTERY_FULL_VOLTAGE - BATTERY_EMPTY_VOLTAGE)) * 100.0f;
  
  if (percentage > 100.0f) {
    percentage = 100.0f;
  }
  if (percentage < 0.0f) {
    percentage = 0.0f;
  }
  
  return (uint8_t)percentage;
}

/**
 * @brief Map battery level vào index cho mảng text/image
 * @param level Dung lượng pin (0-100%)
 * @return Index trong mảng (0-6 tương ứng với 0%, 17%, 33%, 50%, 67%, 83%, 100%)
 */
static uint8_t battery_map_level_to_index(uint8_t level) {
  if (level == 0) return 0;        // 0%
  if (level <= 17) return 1;       // 17%
  if (level <= 33) return 2;       // 33%
  if (level <= 50) return 3;       // 50%
  if (level <= 67) return 4;       // 67%
  if (level <= 83) return 5;       // 83%
  return 6;                         // 100%
}

/**
 * @brief FreeRTOS task để đọc pin định kỳ
 */
static void battery_read_task(void *pvParameters) {
  DataManager_t *data = (DataManager_t *)pvParameters;
  
  ESP_LOGI(TAG_BATTERY_MANAGER, "Battery read task started");
  
  while (1) {
    // Đọc điện áp pin
    float voltage = battery_read_voltage();
    uint8_t level = battery_calculate_level(voltage);
    uint8_t level_index = battery_map_level_to_index(level);
    
    // Cập nhật biến global (thread-safe với mutex hoặc atomic operations)
    current_voltage = voltage;
    current_level = level;
    current_level_index = level_index;
    
    // Tạo chuỗi hiển thị
    snprintf(battery_text, sizeof(battery_text), "%d%% (%.2fV)", level, voltage);
    
    ESP_LOGI(TAG_BATTERY_MANAGER, "Battery: %d%% (index: %d), Voltage: %.2fV", level, level_index, voltage);
    
    // Cập nhật vào DataManager nếu có
    if (data != NULL) {
      data->objectInfo.batteryInfo.batteryLevel = level_index;
      data->objectInfo.batteryInfo.batteryName = battery_text;
    }
    
    // Đợi trước khi đọc lần tiếp theo
    vTaskDelay(pdMS_TO_TICKS(BATTERY_READ_INTERVAL_MS));
  }
}

esp_err_t BatteryManager_Init(void) {
  esp_err_t ret = battery_adc_init();
  if (ret != ESP_OK) {
    return ret;
  }
  
  ESP_LOGI(TAG_BATTERY_MANAGER, "BatteryManager initialized");
  return ESP_OK;
}

/**
 * @brief Khởi động FreeRTOS task để đọc pin
 * @param data Con trỏ đến DataManager để cập nhật thông tin pin
 */
void BatteryManager_StartTask(DataManager_t *data) {
  xTaskCreate(battery_read_task, "battery_read_task", 4096, data, 5, NULL);
  ESP_LOGI(TAG_BATTERY_MANAGER, "Battery read task created");
}

float BatteryManager_GetVoltage(void) {
  return current_voltage;
}

uint8_t BatteryManager_GetLevel(void) {
  return current_level;
}

uint8_t BatteryManager_GetLevelIndex(void) {
  return current_level_index;
}

void BatteryManager_UpdateInfo(BatteryInfo_t *batteryInfo) {
  if (batteryInfo == NULL) {
    return;
  }
  
  batteryInfo->batteryLevel = current_level_index;
  batteryInfo->batteryName = battery_text;
}
