## MRS_Project

### Tổng quan
- Ứng dụng ESP-IDF (ESP32/ESP32‑Cx) hiển thị UI trên OLED SSD1306, quản lý menu, Wi‑Fi, và đọc dữ liệu cảm biến (BME280, …).
- Mã nguồn được tổ chức theo domain: core, ui, drivers, network, sensors, utils.

### Mục lục
1. Giới thiệu nhanh
2. Cấu trúc thư mục
3. Yêu cầu và Build
4. Cấu hình (SPIFFS, I2C, Kconfig)
5. Quy ước mã nguồn
6. Thêm Cảm biến mới
7. Cách sử dụng Hệ thống Cảm biến
8. Thêm Component mới (không phải cảm biến)
9. Gợi ý mở rộng
10. Khắc phục sự cố nhanh

### 1) Giới thiệu nhanh
- UI: menu điều hướng, hiển thị dữ liệu cảm biến theo từng trường và đơn vị.
- Sensors: tầng trung gian đọc/định tuyến dữ liệu tới UI.
- Drivers: i2cdev (khởi tạo I2C idempotent), SSD1306, BME280/BMP280, DS3231/Time.
- Network: Wi‑Fi AP/STA, cấu hình qua giao diện web (SPIFFS).

### 2) Cấu trúc thư mục
- `CMakeLists.txt`: cấu hình build cấp dự án, liệt kê các thư mục component.
- `main/`: entry‑point ứng dụng, khởi tạo hệ thống, tạo task chính.
- `component/`
  - `core/`
    - `DataManager/`: định nghĩa kiểu dữ liệu, trạng thái toàn cục, data context.
    - `FunctionManager/`: callback logic cho menu/chức năng, tạo task đọc cảm biến,…
  - `ui/`
    - `MenuSystem/`: hệ thống menu, điều hướng, liên kết callback.
    - `ScreenManager/`: render UI lên SSD1306 (text, icon, trang dữ liệu).
    - `MenuButton/`: đọc trạng thái nút, phát sinh sự kiện điều hướng.
  - `sensors/`
    - `SensorTypes/`: định nghĩa enum, struct cơ bản (PortId_t, SensorType_t, SensorData_t, sensor_driver_t).
    - `SensorRegistry/`: đăng ký và quản lý các sensor driver, cung cấp API truy cập.
  - `drivers/`
    - `i2cdev/`: khởi tạo I2C dùng chung (idempotent), tiện ích truy cập I2C.
    - `ssd1306/`: driver OLED.
    - `SensorConfig/`: lớp cấu hình và wrapper cho các sensor driver, đồng bộ cấu hình trong lớp driver.
    - `BME280/`, `BMP280/`, `DS3231/`, `Time/`, `esp_idf_lib_helpers/`.
  - `network/`
    - `WifiManager/`: cấu hình/kết nối Wi‑Fi, HTTP handler.
    - `WebConfigWifi/`: tài nguyên SPIFFS (HTML) cấu hình Wi‑Fi.
  - `utils/`
    - `BitManager/`: tiện ích bit/byte chung.

### 3) Yêu cầu và Build
- Yêu cầu: ESP‑IDF v5.x, đã export `IDF_PATH`.
- Lệnh gợi ý:
  - `idf.py set-target esp32c6` (hoặc chip bạn dùng)
  - `idf.py menuconfig` (thiết lập I2C, Wi‑Fi,… nếu có Kconfig)
  - `idf.py build`
  - `idf.py -p COMx flash monitor`

### 4) Cấu hình
- **SPIFFS**: tạo từ `component/network/WebConfigWifi` (đã cấu hình trong `CMakeLists.txt`).
- **I2C**: dùng `drivers/i2cdev`, khởi tạo dùng chung, idempotent; SDA/SCL/Port/Clock cấu hình qua Kconfig.
- **Kconfig**: mỗi component có thể cung cấp `Kconfig.projbuild`; mở `idf.py menuconfig` để điều chỉnh.

### 5) Quy ước mã nguồn
- Phân lớp rõ ràng: `ui` (hiển thị/điều hướng), `sensors` (điều phối đọc), `drivers` (driver thiết bị), `core` (dữ liệu/chức năng), `network` (Wi‑Fi/Web), `utils` (tiện ích).
- Callback UI tập trung ở `FunctionManager` (core); `MenuSystem` chỉ gán callback và context.
- Tránh overflow/ghi vượt mảng; luôn kiểm tra giới hạn chỉ số trước khi truy cập.
- Khởi tạo idempotent cho mọi hạ tầng (I2C, Wi‑Fi, …) để gọi lặp an toàn.

### 6) Thêm Cảm biến mới

Hệ thống sử dụng kiến trúc 3 lớp để quản lý cảm biến:
- **SensorTypes**: Định nghĩa enum và struct cơ bản
- **SensorRegistry**: Đăng ký và quản lý danh sách cảm biến
- **SensorConfig**: Wrapper functions cho các driver cụ thể

#### 6.1) Thêm enum vào SensorTypes.h

Mở `component/sensors/SensorTypes/SensorTypes.h` và thêm enum mới vào `SensorType_t`:

```c
typedef enum {
  SENSOR_NONE = -1,
  SENSOR_BME280 = 0,
  // ... các cảm biến khác
  SENSOR_MY_NEW_SENSOR = 13,  // Thêm enum mới
} SensorType_t;
```

#### 6.2) Tạo Wrapper Functions trong SensorConfig

Mở `component/drivers/SensorConfig/SensorConfig.h` và thêm khai báo:

```c
void sensor_my_new_sensor_init(void);
void sensor_my_new_sensor_read(SensorData_t *data);
void sensor_my_new_sensor_deinit(void);
```

Mở `component/drivers/SensorConfig/SensorConfig.c` và implement:

```c
void sensor_my_new_sensor_init(void) {
  // Khởi tạo sensor của bạn
  // Ví dụ: i2c_init, gpio_config, etc.
}

void sensor_my_new_sensor_read(SensorData_t *data) {
  // Đọc dữ liệu từ sensor
  // Lưu vào data->data_fl[0], data->data_fl[1], ...
  // Hoặc data->data_uint32[], data->data_uint16[], tùy loại dữ liệu
}

void sensor_my_new_sensor_deinit(void) {
  // Giải phóng tài nguyên nếu cần
}
```

#### 6.3) Đăng ký vào SensorRegistry

Mở `component/sensors/SensorRegistry/SensorRegistry.c` và thêm vào mảng `sensor_drivers[]`:

```c
static sensor_driver_t sensor_drivers[] = {
    // ... các cảm biến hiện có
    {
        .name = "My New Sensor",           // Tên hiển thị trong menu
        .init = sensor_my_new_sensor_init, // Hàm khởi tạo
        .read = sensor_my_new_sensor_read, // Hàm đọc dữ liệu
        .deinit = sensor_my_new_sensor_deinit, // Hàm giải phóng (có thể NULL)
        .description = {"Field1", "Field2"},   // Tên các trường dữ liệu
        .unit = {"unit1", "unit2"},           // Đơn vị đo
        .unit_count = 2,                       // Số lượng trường
        .is_init = false,                      // Trạng thái khởi tạo
    },
};
```

Cập nhật hàm `sensor_type_to_name()` để thêm case mới:

```c
const char *sensor_type_to_name(SensorType_t t) {
  switch (t) {
    // ... các case hiện có
    case SENSOR_MY_NEW_SENSOR:
      return "My New Sensor";
    default:
      return "Unknown";
  }
}
```

#### 6.4) Ví dụ: Thêm cảm biến với hàm NULL

Nếu cảm biến chưa có implementation, có thể đăng ký với các hàm NULL:

```c
{
    .name = "MQ-2",
    .init = NULL,      // Chưa implement
    .read = NULL,      // Chưa implement
    .deinit = NULL,    // Chưa implement
    .description = {"Gas"},
    .unit = {"ppm"},
    .unit_count = 1,
    .is_init = false,
},
```

Cảm biến này sẽ xuất hiện trong menu nhưng chưa thể sử dụng cho đến khi implement các hàm.

### 7) Cách sử dụng Hệ thống Cảm biến

#### 7.1) Lấy danh sách cảm biến

```c
#include "SensorRegistry.h"

// Lấy số lượng cảm biến đã đăng ký
size_t count = sensor_registry_get_count();

// Lấy mảng các driver
sensor_driver_t *drivers = sensor_registry_get_drivers();

// Lấy driver cụ thể theo loại
sensor_driver_t *driver = sensor_registry_get_driver(SENSOR_BME280);
if (driver != NULL) {
    // Sử dụng driver
}
```

#### 7.2) Chọn cảm biến cho Port

Hệ thống tự động tạo menu cho việc chọn cảm biến:
1. Vào menu "Sensors" → "Port Config"
2. Chọn Port (Port 1, Port 2, Port 3)
3. Chọn cảm biến từ danh sách (tự động từ SensorRegistry)
4. Hệ thống sẽ tự động khởi tạo cảm biến nếu chưa được init

#### 7.3) Đọc dữ liệu cảm biến

```c
#include "SensorConfig.h"
#include "SensorTypes.h"

SensorData_t data;
sensor_driver_t *driver = sensor_registry_get_driver(SENSOR_BME280);

if (driver != NULL && driver->read != NULL) {
    driver->read(&data);
    // Dữ liệu được lưu trong:
    // - data.data_fl[] cho số thực (float)
    // - data.data_uint32[] cho số nguyên 32-bit
    // - data.data_uint16[] cho số nguyên 16-bit
    // - data.data_uint8[] cho số nguyên 8-bit
}
```

#### 7.4) Hiển thị dữ liệu trên màn hình

```c
#include "ScreenManager.h"

sensor_driver_t *driver = sensor_registry_get_driver(SENSOR_BME280);
SensorData_t data;
driver->read(&data);

// Hiển thị với tên trường, giá trị, và đơn vị
ScreenShowDataSensor(
    driver->description,  // Tên các trường
    data.data_fl,         // Mảng giá trị (float)
    driver->unit,         // Đơn vị
    driver->unit_count    // Số lượng trường
);
```

#### 7.5) Pagination trong Menu

Khi có nhiều hơn 4 cảm biến, menu tự động hỗ trợ phân trang:
- **Items 0-3**: Hiển thị cảm biến 0, 1, 2, 3
- **Khi chọn item 4**: Màn hình tự động scroll để hiển thị từ cảm biến 4 trở đi
- **Tiếp tục scroll**: Mỗi lần chọn item mới >= 4, màn hình sẽ hiển thị 4 items tiếp theo

### 8) Thêm Component mới (không phải cảm biến)
1. Tạo thư mục dưới domain phù hợp, ví dụ `component/ui/MyWidget`.
2. Thêm `CMakeLists.txt` với `idf_component_register(...)` và `REQUIRES` chính xác.
3. Nếu cần cấu hình, thêm `Kconfig.projbuild` trong thư mục component.
4. Sử dụng ở nơi khác qua `REQUIRES` và include header tương ứng.

### 9) Gợi ý mở rộng
- Viết `README.md` ngắn trong mỗi component mô tả API, dependency, Kconfig.
- Thêm `clang-format`/`clang-tidy` và pre‑commit để chuẩn hóa style.
- Thêm test cho logic thuần (mock hardware) nếu có CI.

### 10) Khắc phục sự cố nhanh
- Build lỗi “không tìm thấy component”: kiểm tra đã thêm đường dẫn domain trong `EXTRA_COMPONENT_DIRS` và `REQUIRES` đúng tên component.
- Lỗi I2C driver “not installed”: đảm bảo gọi khởi tạo I2C dùng chung trước khi dùng sensor/OLED và không cài đặt trùng lặp.
- UI hiển thị sai sau thao tác menu: kiểm tra chỉ số mảng bám theo `NUM_PORTS`/`sensor_registry_get_count()` trước khi ghi.


