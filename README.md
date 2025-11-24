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

Dự án được tổ chức theo kiến trúc phân lớp (layered architecture) với các domain rõ ràng:

```
MRS_Project/
├── CMakeLists.txt              # Cấu hình build cấp dự án, khai báo EXTRA_COMPONENT_DIRS
├── sdkconfig                    # File cấu hình ESP-IDF (tự động tạo bởi menuconfig)
├── partitions.csv               # Bảng phân vùng flash (SPIFFS, app, etc.)
├── README.md                    # Tài liệu dự án (file này)
│
├── main/                        # Entry point của ứng dụng
│   ├── CMakeLists.txt          # Cấu hình build cho main component
│   ├── MRS_Project.c           # Hàm app_main(), khởi tạo hệ thống, tạo tasks
│   └── MRS_Project.h           # Header file cho main
│
└── component/                   # Thư mục chứa tất cả các component
    │
    ├── core/                    # ═══════════════════════════════════════
    │   │                        # LỚP CORE: Logic nghiệp vụ và quản lý dữ liệu
    │   │                        # ═══════════════════════════════════════
    │   ├── DataManager/         # Quản lý dữ liệu toàn cục
    │   │   ├── CMakeLists.txt
    │   │   ├── DataManager.h    # Định nghĩa DataManager_t, ScreenState_t, ObjectInfo_t
    │   │   └── DataManager.c    # Khởi tạo và quản lý trạng thái ứng dụng
    │   │
    │   └── FunctionManager/     # Xử lý callback và logic nghiệp vụ
    │       ├── CMakeLists.txt
    │       ├── FunctionManager.h # Khai báo các callback functions
    │       └── FunctionManager.c # Implement: select_sensor_cb, reset_all_ports_callback,
    │                              #           tạo task đọc cảm biến, xử lý menu actions
    │
    ├── ui/                      # ═══════════════════════════════════════
    │   │                        # LỚP UI: Giao diện người dùng và điều hướng
    │   │                        # ═══════════════════════════════════════
    │   ├── MenuSystem/          # Hệ thống menu phân cấp
    │   │   ├── CMakeLists.txt
    │   │   ├── MenuSystem.h     # Định nghĩa menu_t, menu_item_t, menu callback
    │   │   └── MenuSystem.c     # Điều hướng menu, render menu items, pagination
    │   │                         # Tự động tạo menu từ SensorRegistry
    │   │
    │   ├── ScreenManager/       # Quản lý hiển thị trên OLED SSD1306
    │   │   ├── CMakeLists.txt
    │   │   ├── ScreenManager.h  # API: ScreenManagerInit, ScreenShowDataSensor, MenuRender
    │   │   └── ScreenManager.c  # Render text, icons, hiển thị dữ liệu cảm biến tuần tự
    │   │                         # (mỗi field 300ms, font size 16, căn giữa)
    │   │
    │   └── MenuButton/           # Đọc trạng thái nút bấm
    │       ├── CMakeLists.txt
    │       ├── MenuButton.h      # API: MenuButtonInit, ReadButtonStatus
    │       └── MenuButton.c     # Đọc GPIO, phát sinh sự kiện BTN_UP/DOWN/SEL/BACK
    │                            # Tích hợp LED RGB: sáng LED khi bấm nút
    │
    ├── sensors/                 # ═══════════════════════════════════════
    │   │                        # LỚP SENSOR: Định nghĩa và quản lý cảm biến
    │   │                        # ═══════════════════════════════════════
    │   ├── SensorTypes/         # Định nghĩa kiểu dữ liệu cơ bản
    │   │   ├── CMakeLists.txt
    │   │   └── SensorTypes.h    # Enum: PortId_t, SensorType_t
    │   │                         # Struct: SensorData_t, sensor_driver_t
    │   │                         # Định nghĩa tất cả loại cảm biến (BME280, MQ series, ...)
    │   │
    │   └── SensorRegistry/       # Đăng ký và quản lý danh sách cảm biến
    │       ├── CMakeLists.txt
    │       ├── SensorRegistry.h # API: sensor_registry_get_count, sensor_registry_get_drivers
    │       └── SensorRegistry.c # Mảng sensor_drivers[] chứa tất cả cảm biến đã đăng ký
    │                             # Hàm helper: sensor_type_to_name, get_driver, get_count
    │
    ├── drivers/                 # ═══════════════════════════════════════
    │   │                        # LỚP DRIVER: Driver phần cứng và wrapper
    │   │                        # ═══════════════════════════════════════
    │   ├── i2cdev/              # Driver I2C dùng chung (idempotent)
    │   │   ├── CMakeLists.txt
    │   │   ├── Kconfig.projbuild # Cấu hình GPIO SDA/SCL, port, clock
    │   │   ├── i2cdev.h         # API: i2cInitDevCommon, i2c_dev_t
    │   │   └── i2cdev.c         # Khởi tạo I2C master, tiện ích đọc/ghi I2C
    │   │
    │   ├── ssd1306/             # Driver màn hình OLED SSD1306
    │   │   ├── CMakeLists.txt
    │   │   ├── ssd1306.h        # API: ssd1306_create, ssd1306_draw_string, etc.
    │   │   ├── ssd1306.c        # Giao tiếp I2C với OLED, render pixel/text
    │   │   ├── ssd1306_fonts.h  # Font bitmap
    │   │   └── ssd1306_fonts.c  # Dữ liệu font
    │   │
    │   ├── SensorConfig/        # Wrapper và cấu hình cho sensor drivers
    │   │   ├── CMakeLists.txt
    │   │   ├── SensorConfig.h   # Wrapper API: sensor_bme280_init/read/deinit
    │   │   └── SensorConfig.c   # Implement wrapper, gọi driver thực tế (BME280, ...)
    │   │                         # Đồng bộ cấu hình trong lớp driver
    │   │
    │   ├── BME280/              # Driver cảm biến BME280 (nhiệt độ, độ ẩm, áp suất)
    │   │   ├── CMakeLists.txt
    │   │   ├── Kconfig.projbuild
    │   │   ├── bme280.h
    │   │   └── bme280.c
    │   │
    │   ├── BMP280/              # Driver cảm biến BMP280 (nhiệt độ, áp suất)
    │   │   ├── CMakeLists.txt
    │   │   ├── bmp280.h
    │   │   └── bmp280.c
    │   │
    │   ├── DS3231/              # Driver RTC DS3231 (thời gian thực)
    │   │   ├── CMakeLists.txt
    │   │   ├── ds3231.h
    │   │   └── ds3231.c
    │   │
    │   ├── Time/                # Wrapper cho DS3231, quản lý thời gian
    │   │   ├── CMakeLists.txt
    │   │   ├── Kconfig.projbuild
    │   │   ├── DS3231Time.h
    │   │   └── DS3231Time.c
    │   │
    │   ├── LedRGB/              # Driver LED RGB WS2812B (ESP32-C6)
    │   │   ├── CMakeLists.txt
    │   │   ├── Kconfig.projbuild # ACTIVE_LED_RGB (default y cho ESP32-C6)
    │   │   ├── LedRGB.h         # API: LedRGB_Init, LedRGB_SetColor, LedRGB_SetButtonColor
    │   │   └── LedRGB.c         # Sử dụng RMT peripheral, ESP Timer để tắt LED sau delay
    │   │
    │   └── esp_idf_lib_helpers/ # Helper macros cho ESP-IDF lib
    │       ├── CMakeLists.txt
    │       ├── esp_idf_lib_helpers.h
    │       └── ets_sys.h
    │
    ├── network/                 # ═══════════════════════════════════════
    │   │                        # LỚP NETWORK: Wi-Fi và Web
    │   │                        # ═══════════════════════════════════════
    │   ├── WifiManager/         # Quản lý Wi-Fi (AP/STA mode)
    │   │   ├── CMakeLists.txt
    │   │   ├── Kconfig.projbuild # Cấu hình SSID, password, AP/STA
    │   │   ├── WifiManager.h    # API: wifi_init_sta, wifi_init_softap
    │   │   └── WifiManager.c    # Khởi tạo Wi-Fi, HTTP server, xử lý kết nối
    │   │
    │   └── WebConfigWifi/       # Tài nguyên web cho cấu hình Wi-Fi
    │       ├── index.html       # Trang cấu hình Wi-Fi (SPIFFS)
    │       └── redirect.html    # Trang redirect sau khi cấu hình
    │
    └── utils/                   # ═══════════════════════════════════════
        │                        # LỚP UTILS: Tiện ích chung
        │                        # ═══════════════════════════════════════
        └── BitManager/          # Tiện ích xử lý bit/byte
            ├── CMakeLists.txt
            ├── BitManager.h
            └── BitManager.c
```

#### Giải thích chi tiết các lớp:

**1. Lớp Core (`component/core/`)**
- **Mục đích**: Chứa logic nghiệp vụ và quản lý dữ liệu toàn cục
- **DataManager**: Lưu trữ trạng thái ứng dụng (selected sensors, screen state, object info)
- **FunctionManager**: Xử lý các callback từ menu, tạo tasks đọc cảm biến, reset ports

**2. Lớp UI (`component/ui/`)**
- **Mục đích**: Giao diện người dùng và điều hướng
- **MenuSystem**: Hệ thống menu phân cấp, tự động tạo menu từ SensorRegistry, hỗ trợ pagination
- **ScreenManager**: Render UI lên OLED (text, icons, dữ liệu cảm biến)
- **MenuButton**: Đọc GPIO nút bấm, tích hợp LED RGB feedback

**3. Lớp Sensors (`component/sensors/`)**
- **Mục đích**: Định nghĩa và quản lý danh sách cảm biến
- **SensorTypes**: Định nghĩa enum và struct cơ bản (PortId_t, SensorType_t, SensorData_t)
- **SensorRegistry**: Đăng ký tất cả cảm biến, cung cấp API truy cập danh sách

**4. Lớp Drivers (`component/drivers/`)**
- **Mục đích**: Driver phần cứng và wrapper functions
- **i2cdev**: Khởi tạo I2C dùng chung (idempotent)
- **ssd1306**: Driver màn hình OLED
- **SensorConfig**: Wrapper functions cho các sensor driver cụ thể
- **BME280, BMP280, DS3231**: Driver cảm biến cụ thể
- **LedRGB**: Driver LED RGB WS2812B (ESP32-C6), sử dụng RMT peripheral

**5. Lớp Network (`component/network/`)**
- **Mục đích**: Quản lý Wi-Fi và web interface
- **WifiManager**: Khởi tạo Wi-Fi AP/STA, HTTP server
- **WebConfigWifi**: Tài nguyên HTML cho cấu hình Wi-Fi qua web

**6. Lớp Utils (`component/utils/`)**
- **Mục đích**: Tiện ích chung
- **BitManager**: Xử lý bit/byte operations

#### Luồng dữ liệu:

```
User Input (Button) 
    ↓
MenuButton → MenuSystem → FunctionManager
    ↓                              ↓
ScreenManager ← SensorRegistry ← SensorConfig ← Driver (BME280, etc.)
    ↓
SSD1306 OLED Display
```

#### Dependency Flow:

- **UI Layer** phụ thuộc vào **Core Layer** và **Sensors Layer**
- **Sensors Layer** phụ thuộc vào **Drivers Layer**
- **Core Layer** phụ thuộc vào **Sensors Layer** và **Drivers Layer**
- **Drivers Layer** độc lập, chỉ phụ thuộc vào ESP-IDF và hardware

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


