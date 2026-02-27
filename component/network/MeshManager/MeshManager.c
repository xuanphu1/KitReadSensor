#include "MeshManager.h"

static const char *TAG_MESH = "MeshManager";
// Con trỏ tới DataManager toàn cục của app (được truyền từ FunctionManager)
static DataManager_t *s_data_manager = NULL;

#define MESH_UDP_PORT 12345

static void mesh_udp_client_task(void *pvParameters)
{
    (void)pvParameters;
    while (1) {
        uint32_t size = 0;
        const node_info_list_t *node = esp_mesh_lite_get_nodes_list(&size);
        const node_info_list_t *target = NULL;

        for (uint32_t loop = 0; (loop < size) && (node != NULL); loop++) {
            if (node->node->level == 1) {
                target = node;
                break;
            }
            node = node->next;
        }

        if (!target) {
            ESP_LOGW(TAG_MESH, "No root node found in mesh list, retry...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(TAG_MESH, "Unable to create socket: errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        struct sockaddr_in dest_addr;
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(MESH_UDP_PORT);
        /* Root node IP trong mạng mesh-lite thường là 192.168.5.1.
         * Nếu cấu hình khác, bạn chỉnh lại IP này cho đúng. */
        dest_addr.sin_addr.s_addr = inet_addr("192.168.5.1");

        char payload[512];
        int offset = 0;

        // Bắt đầu JSON: level + mảng ports
        offset += snprintf(payload + offset, sizeof(payload) - offset,
                           "{\"Node_ID\":%d,\"ports\":[",
                           esp_mesh_lite_get_level());

        bool first_port = true;

        if (s_data_manager != NULL) {
            for (PortId_t port = 0; port < NUM_PORTS; port++) {
                SensorType_t type = s_data_manager->selectedSensor[port];
                if (type == SENSOR_NONE) {
                    continue;
                }

                sensor_driver_t *driver = sensor_registry_get_driver(type);
                if (driver == NULL || driver->read == NULL) {
                    ESP_LOGW(TAG_MESH, "Port %d: invalid driver for sensor %d", port, type);
                    continue;
                }

                SensorData_t data;
                system_err_t ret = driver->read(&data);
                if (ret != MRS_OK) {
                    ESP_LOGW(TAG_MESH, "Port %d: read failed: %s",
                             port, system_err_to_name(ret));
                    continue;
                }

                if (!first_port) {
                    if (offset < (int)sizeof(payload) - 1) {
                        payload[offset++] = ',';
                        payload[offset] = '\0';
                    }
                }
                first_port = false;

                offset += snprintf(payload + offset, sizeof(payload) - offset,
                                   "{\"port\":%d,\"sensor\":\"%s\",\"fields\":{",
                                   port + 1,
                                   driver->name ? driver->name : "unknown");

                bool first_field = true;
                for (uint8_t i = 0; i < driver->unit_count && i < 5; i++) {
                    const char *field_name = driver->description[i];
                    if (!field_name) {
                        continue;
                    }

                    if (!first_field) {
                        if (offset < (int)sizeof(payload) - 1) {
                            payload[offset++] = ',';
                            payload[offset] = '\0';
                        }
                    }
                    first_field = false;

                    float value = data.data_fl[i];
                    offset += snprintf(payload + offset, sizeof(payload) - offset,
                                       "\"%s\":%.3f", field_name, value);

                    if (offset >= (int)sizeof(payload) - 1) {
                        break;
                    }
                }

                if (offset < (int)sizeof(payload) - 1) {
                    payload[offset++] = '}';
                    payload[offset++] = '}';
                    payload[offset] = '\0';
                }

                if (offset >= (int)sizeof(payload) - 1) {
                    ESP_LOGW(TAG_MESH, "JSON payload truncated");
                    break;
                }
            }
        }

        if (offset < (int)sizeof(payload) - 2) {
            offset += snprintf(payload + offset, sizeof(payload) - offset, "]}");
        } else {
            // đảm bảo chuỗi kết thúc hợp lệ
            payload[sizeof(payload) - 2] = '}';
            payload[sizeof(payload) - 1] = '\0';
        }

        ESP_LOGI(TAG_MESH, "Sending JSON to 192.168.5.1:%d, payload: %s",
                 MESH_UDP_PORT, payload);

        int err = sendto(sock, payload, strlen(payload), 0,
                         (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        if (err < 0) {
            ESP_LOGE(TAG_MESH, "Error occurred during sending: errno %d", errno);
        } else {
            ESP_LOGI(TAG_MESH, "Message sent successfully (%d bytes)", err);
        }

        close(sock);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static void mesh_print_system_info_timercb(TimerHandle_t timer)
{
    (void)timer;
    /* Không làm việc nặng trong Timer Service task để tránh tràn stack.
     * Chỉ log thông tin cơ bản của mesh. */
    ESP_LOGI(TAG_MESH, "Mesh info: level=%d, free_heap=%"PRIu32,
             esp_mesh_lite_get_level(), esp_get_free_heap_size());
}


static void mesh_wifi_init(void)
{
    // Station
    wifi_config_t wifi_config;
    memset(&wifi_config, 0x0, sizeof(wifi_config_t));
    esp_bridge_wifi_set_config(WIFI_IF_STA, &wifi_config);

    // SoftAP
    wifi_config_t wifi_softap_config = {
        .ap = {
            .ssid     = CONFIG_BRIDGE_SOFTAP_SSID,
            .password = CONFIG_BRIDGE_SOFTAP_PASSWORD,
            .channel  = 11,
        },
    };
    esp_bridge_wifi_set_config(WIFI_IF_AP, &wifi_softap_config);
}

static void mesh_app_wifi_set_softap_info(void)
{
    char softap_ssid[33];
    char softap_psw[64];
    uint8_t softap_mac[6];
    size_t ssid_size = sizeof(softap_ssid);
    size_t psw_size = sizeof(softap_psw);
    esp_wifi_get_mac(WIFI_IF_AP, softap_mac);
    memset(softap_ssid, 0x0, sizeof(softap_ssid));
    memset(softap_psw, 0x0, sizeof(softap_psw));

    if (esp_mesh_lite_get_softap_ssid_from_nvs(softap_ssid, &ssid_size) == ESP_OK) {
        ESP_LOGI(TAG_MESH, "Get ssid from nvs: %s", softap_ssid);
    } else {
#ifdef CONFIG_BRIDGE_SOFTAP_SSID_END_WITH_THE_MAC
        snprintf(softap_ssid, sizeof(softap_ssid), "%.25s_%02x%02x%02x",
                 CONFIG_BRIDGE_SOFTAP_SSID, softap_mac[3], softap_mac[4], softap_mac[5]);
#else
        snprintf(softap_ssid, sizeof(softap_ssid), "%.32s", CONFIG_BRIDGE_SOFTAP_SSID);
#endif
        ESP_LOGI(TAG_MESH, "Get ssid from nvs failed, set ssid: %s", softap_ssid);
    }

    if (esp_mesh_lite_get_softap_psw_from_nvs(softap_psw, &psw_size) == ESP_OK) {
        ESP_LOGI(TAG_MESH, "Get psw from nvs: [HIDDEN]");
    } else {
        strlcpy(softap_psw, CONFIG_BRIDGE_SOFTAP_PASSWORD, sizeof(softap_psw));
        ESP_LOGI(TAG_MESH, "Get psw from nvs failed, set psw: [HIDDEN]");
    }

    esp_mesh_lite_set_softap_info(softap_ssid, softap_psw);
}

void MeshManager_StartMeshClient(DataManager_t *data)
{
    static bool started = false;
    if (started) {
        ESP_LOGI(TAG_MESH, "Mesh client already started");
        return;
    }

    // Lưu con trỏ DataManager để task mesh có thể đọc dữ liệu cảm biến
    s_data_manager = data;

    ESP_LOGI(TAG_MESH, "Starting mesh-lite client...");

    // NVS, netif, event loop đã được khởi tạo trong main/WifiManager
    // Không gọi esp_bridge_create_all_netif() nữa để tránh tạo lại default WiFi AP/STA netif

    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t err = esp_event_loop_create_default();
    if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG_MESH, "Event loop already exists, skipping...");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG_MESH, "Failed to create event loop: %s", esp_err_to_name(err));
        // Tùy chọn: return; nếu đây là lỗi nghiêm trọng không phải do trùng lặp
    }

    esp_bridge_create_all_netif();
    mesh_wifi_init();

    esp_mesh_lite_config_t mesh_lite_config = ESP_MESH_LITE_DEFAULT_INIT();
    mesh_lite_config.join_mesh_ignore_router_status     = true;
    mesh_lite_config.join_mesh_without_configured_wifi  = true;
    esp_mesh_lite_init(&mesh_lite_config);

    mesh_app_wifi_set_softap_info();

    ESP_LOGI(TAG_MESH, "Child node");
    esp_mesh_lite_set_disallowed_level(1);

    esp_mesh_lite_start();
    /* UDP client task for child */
    xTaskCreate(mesh_udp_client_task, "mesh_udp_client", 4096, NULL, 5, &data->TaskHandle_Array[TASK_MESH_UDP_CLIENT]);

    TimerHandle_t timer = xTimerCreate("mesh_print_system_info", 10000 / portTICK_PERIOD_MS,
                                       pdTRUE, NULL, mesh_print_system_info_timercb);
    if (timer != NULL) {
        xTimerStart(timer, 0);
    }

    started = true; 
}
