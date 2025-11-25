#include "WifiManager.h"
#include "Common.h"

// WiFi variables
EventGroupHandle_t wifi_event_group;
bool ap_mode_active = false;
char pending_ssid[33];
char pending_password[65];
bool wifi_connect_pending = false;
bool wifi_initialized = false;
esp_netif_t *sta_netif = NULL;
esp_netif_t *ap_netif = NULL;
SemaphoreHandle_t wifi_mutex = NULL;
int retry_num = 0;
httpd_handle_t server = NULL;
TaskHandle_t dns_server_task_handle = NULL;
static const char *TAG = "WiFiManager";



system_err_t start_webserver(void);
system_err_t stop_webserver(void);
system_err_t init_spiffs(void);
system_err_t root_handler(httpd_req_t *req);
system_err_t wifi_config_handler(httpd_req_t *req);
system_err_t wifi_status_handler(httpd_req_t *req);
system_err_t redirect_handler(httpd_req_t *req);
system_err_t generate_204_handler(httpd_req_t *req);
system_err_t common_get_handler(httpd_req_t *req, httpd_err_code_t error);
system_err_t captive_portal_handler(httpd_req_t *req);
void dns_server_task(void *pvParameters);

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (retry_num < CONFIG_MAX_RETRY) {
      esp_wifi_disconnect();
      esp_wifi_connect();
      retry_num++;
      ESP_LOGI(TAG, "Retry to connect to the AP");
    } else {
      xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "Connect to the AP failed");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    retry_num = 0;
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

system_err_t wifi_init_common(void) {
  if (wifi_initialized) {
    ESP_LOGI(TAG, "WiFi already initialized");
    return MRS_OK;
  }

  // Initialize TCP/IP stack and event loop only once
  esp_err_t ret = esp_netif_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize netif: %s", esp_err_to_name(ret));
    return MRS_ERR_WIFIMANAGER_INIT_FAILED;
  }

  ret = esp_event_loop_create_default();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(ret));
    return MRS_ERR_WIFIMANAGER_INIT_FAILED;
  }

  // Create both netifs if they don't exist
  if (sta_netif == NULL) {
    sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
      ESP_LOGE(TAG, "Failed to create STA netif");
      return MRS_ERR_WIFIMANAGER_STA_FAILED;
    }
  }

  if (ap_netif == NULL) {
    ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == NULL) {
      ESP_LOGE(TAG, "Failed to create AP netif");
      return MRS_ERR_WIFIMANAGER_AP_FAILED;
    }
  }

  // Initialize WiFi
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ret = esp_wifi_init(&cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
    return MRS_ERR_WIFIMANAGER_INIT_FAILED;
  }

  ret = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set WiFi storage: %s", esp_err_to_name(ret));
    return MRS_ERR_WIFIMANAGER_INIT_FAILED;
  }

  // Register event handlers
  ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                    &wifi_event_handler, NULL);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
    return MRS_ERR_WIFIMANAGER_INIT_FAILED;
  }

  ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                    &wifi_event_handler, NULL);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
    return MRS_ERR_WIFIMANAGER_INIT_FAILED;
  }

  wifi_initialized = true;
  return MRS_OK;
}

void wifi_init_sta(void *pvParameters) {
  ESP_ERROR_CHECK(init_spiffs());
  do {
    wifi_event_group = xEventGroupCreate();
    wifi_mutex = xSemaphoreCreateMutex();

    wifi_init_common();

    // Kiểm tra xem có cấu hình WiFi mặc định không
    if (strlen(CONFIG_WIFI_SSID) > 0) {
      ESP_LOGI(TAG, "Attempting to connect to default WiFi: %s",
               CONFIG_WIFI_SSID);

      // Thử kết nối với cấu hình mặc định
      wifi_config_t wifi_config = {
          .sta =
              {
                  .ssid = CONFIG_WIFI_SSID,
                  .password = CONFIG_WIFI_PASS,
                  .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                  .pmf_cfg = {.capable = true, .required = false},
              },
      };

      if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));
        ESP_ERROR_CHECK(esp_wifi_start());
        xSemaphoreGive(wifi_mutex);
      }

      // Chờ kết nối thành công hoặc thất bại
      if (xEventGroupWaitBits(wifi_event_group,
                              WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE,
                              pdFALSE, pdMS_TO_TICKS(10000)) &
          WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to SSID:%s", CONFIG_WIFI_SSID);
        strncpy(pending_ssid, CONFIG_WIFI_SSID, sizeof(pending_ssid) - 1);
        pending_ssid[sizeof(pending_ssid) - 1] = '\0';
        vTaskDelete(NULL);

      } else {
        ESP_LOGI(TAG, "Failed to connect to default SSID:%s", CONFIG_WIFI_SSID);
        vTaskDelete(NULL);
      }
    } else {
      ESP_LOGI(TAG, "No default WiFi configuration found");
      vTaskDelete(NULL);
    }
    vTaskDelete(NULL);
  } while (0);
}
void dns_server_task(void *pvParameters) {
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  uint8_t rx_buffer[DNS_MAX_LEN];
  uint8_t tx_buffer[DNS_MAX_LEN];
  char *ip_addr = "192.168.4.1";

  if (sock < 0) {
    ESP_LOGE(TAG, "Failed to create socket");
    vTaskDelete(NULL);
    return;
  }

  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(DNS_PORT);

  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    ESP_LOGE(TAG, "Failed to bind socket");
    close(sock);
    vTaskDelete(NULL);
    return;
  }

  while (1) {
    socklen_t socklen = sizeof(client_addr);
    int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0,
                       (struct sockaddr *)&client_addr, &socklen);

    if (len > 0) {
      // Copy header and question from request
      memcpy(tx_buffer, rx_buffer, len);

      // Set response flags
      tx_buffer[2] |= 0x80; // Set QR bit
      tx_buffer[3] = 0x80;  // No error
      tx_buffer[7] = 1;     // One answer

      // Skip question section
      int question_len = strlen((char *)(rx_buffer + 12)) + 1;
      int offset = 12 + question_len + 4;

      // Add answer section
      tx_buffer[offset++] = 0xC0; // Name pointer to question
      tx_buffer[offset++] = 0x0C;
      tx_buffer[offset++] = 0x00; // Type A
      tx_buffer[offset++] = 0x01;
      tx_buffer[offset++] = 0x00; // Class IN
      tx_buffer[offset++] = 0x01;
      tx_buffer[offset++] = 0x00; // TTL (4 bytes)
      tx_buffer[offset++] = 0x00;
      tx_buffer[offset++] = 0x00;
      tx_buffer[offset++] = 0x3C;
      tx_buffer[offset++] = 0x00; // Data length
      tx_buffer[offset++] = 0x04;

      // Add IP address
      inet_pton(AF_INET, ip_addr, tx_buffer + offset);
      offset += 4;

      // Send response
      sendto(sock, tx_buffer, offset, 0, (struct sockaddr *)&client_addr,
             sizeof(client_addr));
    }
  }
}

system_err_t wifi_init_ap(void) {
  ESP_LOGI(TAG, "Initializing WiFi AP mode");
  
  system_err_t ret;
  if (!wifi_initialized) {
    ret = wifi_init_common();
    if (ret != MRS_OK) {
      return ret;
    }
  }

  // Stop DHCP server
  esp_err_t esp_ret = esp_netif_dhcps_stop(ap_netif);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to stop DHCP server: %s", esp_err_to_name(esp_ret));
    return MRS_ERR_WIFIMANAGER_AP_FAILED;
  }

  // Configure IP info
  esp_netif_ip_info_t ip_info = {.ip.addr = ipaddr_addr("192.168.4.1"),
                                 .netmask.addr = ipaddr_addr("255.255.255.0"),
                                 .gw.addr = ipaddr_addr("192.168.4.1")};
  esp_ret = esp_netif_set_ip_info(ap_netif, &ip_info);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set IP info: %s", esp_err_to_name(esp_ret));
    return MRS_ERR_WIFIMANAGER_AP_FAILED;
  }

  // Configure DNS
  esp_netif_dns_info_t dns_info = {0};
  dns_info.ip.u_addr.ip4.addr = ipaddr_addr("192.168.4.1");
  esp_ret = esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns_info);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set DNS info: %s", esp_err_to_name(esp_ret));
    return MRS_ERR_WIFIMANAGER_AP_FAILED;
  }

  // Restart DHCP server with new configuration
  esp_ret = esp_netif_dhcps_start(ap_netif);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start DHCP server: %s", esp_err_to_name(esp_ret));
    return MRS_ERR_WIFIMANAGER_AP_FAILED;
  }

  wifi_config_t wifi_config = {
      .ap =
          {
              .ssid = AP_SSID,
              .ssid_len = strlen(AP_SSID),
              .password = "",
              .max_connection = AP_MAX_CONN,
              .authmode = WIFI_AUTH_OPEN,
              .channel = AP_CHANNEL,
          },
  };

  esp_ret = esp_wifi_set_mode(WIFI_MODE_AP);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(esp_ret));
    return MRS_ERR_WIFIMANAGER_AP_FAILED;
  }

  esp_ret = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(esp_ret));
    return MRS_ERR_WIFIMANAGER_AP_FAILED;
  }

  esp_ret = esp_wifi_start();
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(esp_ret));
    return MRS_ERR_WIFIMANAGER_AP_FAILED;
  }

  ESP_LOGI(TAG, "WiFi AP started with SSID: %s (No password required)",
           AP_SSID);
  ESP_LOGI(TAG, "AP IP address: 192.168.4.1");

  // Start DNS server for captive portal
  BaseType_t task_ret = xTaskCreate(dns_server_task, "dns_server", 4096, NULL, 5,
                                    &dns_server_task_handle);
  if (task_ret != pdPASS) {
    ESP_LOGE(TAG, "Failed to create DNS server task");
    return MRS_ERR_WIFIMANAGER_AP_FAILED;
  }

  // Start web server for captive portal
  ret = start_webserver();
  if (ret != MRS_OK) {
    ESP_LOGE(TAG, "Failed to start web server");
    if (dns_server_task_handle != NULL) {
      vTaskDelete(dns_server_task_handle);
      dns_server_task_handle = NULL;
    }
    return ret;
  }

  ap_mode_active = true;
  xEventGroupSetBits(wifi_event_group, WIFI_AP_MODE_BIT);
  return MRS_OK;
}

system_err_t wifi_connect_handler(DataManager_t *data) {
  if (!wifi_connect_pending) {
    return MRS_OK;  // No pending connection
  }

  if (data == NULL) {
    ESP_LOGE(TAG, "Invalid parameter: data is NULL");
    wifi_connect_pending = false;
    return MRS_ERR_CORE_INVALID_PARAM;
  }

  // Validate SSID
  if (strlen(pending_ssid) == 0 || strlen(pending_ssid) > 32) {
    ESP_LOGE(TAG, "Invalid SSID length");
    wifi_connect_pending = false;
    return MRS_ERR_WIFIMANAGER_INVALID_SSID;
  }

  // Validate password length (max 64 characters for WPA2)
  if (strlen(pending_password) > 64) {
    ESP_LOGE(TAG, "Invalid password length");
    wifi_connect_pending = false;
    return MRS_ERR_WIFIMANAGER_INVALID_PASS;
  }

  ESP_LOGI(TAG, "Attempting to connect to WiFi: SSID=%s", pending_ssid);
  
  // Take mutex to ensure exclusive access to WiFi initialization
  if (xSemaphoreTake(wifi_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
    ESP_LOGE(TAG, "Failed to acquire WiFi mutex");
    wifi_connect_pending = false;
    return MRS_ERR_NETWORK_TIMEOUT;
  }

  // Stop AP mode and web server
  system_err_t ret = stop_webserver();
  if (ret != MRS_OK) {
    ESP_LOGW(TAG, "Failed to stop web server: %s", system_err_to_name(ret));
  }

  esp_err_t esp_ret = esp_wifi_stop();
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to stop WiFi: %s", esp_err_to_name(esp_ret));
    xSemaphoreGive(wifi_mutex);
    wifi_connect_pending = false;
    return MRS_ERR_WIFIMANAGER_STA_FAILED;
  }

  vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for WiFi to stop
  ap_mode_active = false;
  xEventGroupClearBits(wifi_event_group, WIFI_AP_MODE_BIT);

  // Clear any previous connection bits
  xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
  retry_num = 0;

  // Configure STA mode with new credentials
  wifi_config_t wifi_config = {
      .sta =
          {
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,
              .pmf_cfg = {.capable = true, .required = false},
          },
  };

  strncpy((char *)wifi_config.sta.ssid, pending_ssid,
          sizeof(wifi_config.sta.ssid) - 1);
  wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0';
  
  strncpy((char *)wifi_config.sta.password, pending_password,
          sizeof(wifi_config.sta.password) - 1);
  wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0';

  // Make sure WiFi is initialized
  if (!wifi_initialized) {
    ret = wifi_init_common();
    if (ret != MRS_OK) {
      xSemaphoreGive(wifi_mutex);
      wifi_connect_pending = false;
      return ret;
    }
  }

  esp_ret = esp_wifi_set_mode(WIFI_MODE_STA);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(esp_ret));
    xSemaphoreGive(wifi_mutex);
    wifi_connect_pending = false;
    return MRS_ERR_WIFIMANAGER_STA_FAILED;
  }

  esp_ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set WiFi config: %s", esp_err_to_name(esp_ret));
    xSemaphoreGive(wifi_mutex);
    wifi_connect_pending = false;
    return MRS_ERR_WIFIMANAGER_STA_FAILED;
  }

  esp_ret = esp_wifi_start();
  if (esp_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(esp_ret));
    xSemaphoreGive(wifi_mutex);
    wifi_connect_pending = false;
    return MRS_ERR_WIFIMANAGER_STA_FAILED;
  }

  // Wait for connection
  EventBits_t bits = xEventGroupWaitBits(
      wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE,
      pdFALSE, pdMS_TO_TICKS(10000));

  wifi_connect_pending = false;
  xSemaphoreGive(wifi_mutex); // Release mutex

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "WiFi connected successfully");
    return MRS_OK;
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGE(TAG, "WiFi connection failed");
    return MRS_ERR_NETWORK_CONNECTION_FAILED;
  } else {
    ESP_LOGE(TAG, "WiFi connection timeout");
    return MRS_ERR_NETWORK_TIMEOUT;
  }
}

system_err_t update_wifi_status(wifiInfo_t *WifiInfo) {
  if (WifiInfo == NULL) {
    ESP_LOGE(TAG, "Invalid parameter: WifiInfo is NULL");
    return MRS_ERR_CORE_INVALID_PARAM;
  }

  if (wifi_event_group == NULL) {
    ESP_LOGE(TAG, "WiFi event group not initialized");
    return MRS_ERR_NETWORK_NOT_INITIALIZED;
  }

  EventBits_t bits = xEventGroupGetBits(wifi_event_group);
  if (bits & WIFI_CONNECTED_BIT) {
    WifiInfo->wifiStatus = CONNECTED;
    WifiInfo->wifiName = (char *)pending_ssid;
  } else if (bits & WIFI_FAIL_BIT || bits & WIFI_AP_MODE_BIT) {
    WifiInfo->wifiStatus = DISCONNECTED;
    WifiInfo->wifiName = NULL;
  } else {
    // WiFi is initializing or in unknown state
    WifiInfo->wifiStatus = DISCONNECTED;
    WifiInfo->wifiName = NULL;
  }
  
  // ESP_LOGI(TAG, "Update Wifi Status successfully: %d", WifiInfo->wifiStatus);
  // ESP_LOGI(TAG, "Wifi Name: %s", (char *)WifiInfo->wifiName);
  return MRS_OK;
}

system_err_t init_spiffs(void) {
  ESP_LOGI(TAG, "Initializing SPIFFS");

  esp_vfs_spiffs_conf_t conf = {.base_path = "/spiffs",
                                .partition_label = NULL,
                                .max_files = 5,
                                .format_if_mount_failed = true};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);
  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return MRS_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)",
             esp_err_to_name(ret));
    return MRS_FAIL;
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }

  return MRS_OK;
}

system_err_t root_handler(httpd_req_t *req) {
  if (req == NULL) {
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  FILE *file = fopen("/spiffs/index.html", "r");
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open index.html");
    httpd_resp_send_404(req);
    return MRS_FAIL;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocate buffer for file content
  char *buffer = malloc(file_size + 1);
  if (buffer == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for HTML file");
    fclose(file);
    httpd_resp_send_500(req);
    return MRS_ERR_CORE_OUT_OF_MEMORY;
  }

  // Read file content
  size_t bytes_read = fread(buffer, 1, file_size, file);
  buffer[bytes_read] = '\0';
  fclose(file);

  httpd_resp_set_type(req, "text/html");
  esp_err_t ret = httpd_resp_send(req, buffer, bytes_read);
  free(buffer);
  
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(ret));
    return MRS_FAIL;
  }
  
  return MRS_OK;
}

system_err_t wifi_config_handler(httpd_req_t *req) {
  if (req == NULL) {
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  char content[256];
  int recv_len = httpd_req_recv(req, content, sizeof(content) - 1);
  if (recv_len <= 0) {
    ESP_LOGE(TAG, "Failed to receive request data");
    return MRS_FAIL;
  }
  content[recv_len] = '\0';

  cJSON *root = cJSON_Parse(content);
  if (root == NULL) {
    ESP_LOGE(TAG, "Failed to parse JSON");
    return MRS_FAIL;
  }

  cJSON *ssid_json = cJSON_GetObjectItem(root, "ssid");
  cJSON *password_json = cJSON_GetObjectItem(root, "password");

  if (!cJSON_IsString(ssid_json) || !cJSON_IsString(password_json)) {
    ESP_LOGE(TAG, "Invalid JSON format: ssid or password missing");
    cJSON_Delete(root);
    return MRS_ERR_NETWORK_INVALID_CONFIG;
  }

  char ssid[33];
  char password[65];
  strncpy(ssid, ssid_json->valuestring, sizeof(ssid) - 1);
  strncpy(password, password_json->valuestring, sizeof(password) - 1);
  ssid[sizeof(ssid) - 1] = '\0';
  password[sizeof(password) - 1] = '\0';

  // Validate SSID
  if (strlen(ssid) == 0 || strlen(ssid) > 32) {
    ESP_LOGE(TAG, "Invalid SSID length");
    cJSON_Delete(root);
    return MRS_ERR_WIFIMANAGER_INVALID_SSID;
  }

  // Validate password length
  if (strlen(password) > 64) {
    ESP_LOGE(TAG, "Invalid password length");
    cJSON_Delete(root);
    return MRS_ERR_WIFIMANAGER_INVALID_PASS;
  }

  ESP_LOGI(TAG, "Configuring WiFi: SSID=%s", ssid);

  // Store credentials for connection task
  strncpy(pending_ssid, ssid, sizeof(pending_ssid) - 1);
  strncpy(pending_password, password, sizeof(pending_password) - 1);
  pending_ssid[sizeof(pending_ssid) - 1] = '\0';
  pending_password[sizeof(pending_password) - 1] = '\0';
  wifi_connect_pending = true;

  cJSON *response = cJSON_CreateObject();
  cJSON_AddBoolToObject(response, "success", true);
  cJSON_AddStringToObject(response, "message", "Attempting to connect...");

  char *response_str = cJSON_PrintUnformatted(response);
  httpd_resp_set_type(req, "application/json");
  esp_err_t ret = httpd_resp_send(req, response_str, strlen(response_str));

  free(response_str);
  cJSON_Delete(response);
  cJSON_Delete(root);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(ret));
    return MRS_FAIL;
  }

  return MRS_OK;
}

system_err_t wifi_status_handler(httpd_req_t *req) {
  if (req == NULL) {
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  cJSON *root = cJSON_CreateObject();
  cJSON_AddBoolToObject(root, "ap_mode", ap_mode_active);
  cJSON_AddStringToObject(root, "ap_ssid", AP_SSID);
  cJSON_AddStringToObject(root, "ap_ip", "192.168.4.1");

  char *json_str = cJSON_PrintUnformatted(root);
  if (json_str == NULL) {
    ESP_LOGE(TAG, "Failed to create JSON string");
    cJSON_Delete(root);
    return MRS_ERR_CORE_OUT_OF_MEMORY;
  }

  httpd_resp_set_type(req, "application/json");
  esp_err_t ret = httpd_resp_send(req, json_str, strlen(json_str));

  free(json_str);
  cJSON_Delete(root);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(ret));
    return MRS_FAIL;
  }

  return MRS_OK;
}

system_err_t redirect_handler(httpd_req_t *req) {
  if (req == NULL) {
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  FILE *file = fopen("/spiffs/redirect.html", "r");
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open redirect.html");
    httpd_resp_send_404(req);
    return MRS_FAIL;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocate buffer for file content
  char *buffer = malloc(file_size + 1);
  if (buffer == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for redirect HTML file");
    fclose(file);
    httpd_resp_send_500(req);
    return MRS_ERR_CORE_OUT_OF_MEMORY;
  }

  // Read file content
  size_t bytes_read = fread(buffer, 1, file_size, file);
  buffer[bytes_read] = '\0';
  fclose(file);

  httpd_resp_set_type(req, "text/html");
  esp_err_t ret = httpd_resp_send(req, buffer, bytes_read);
  free(buffer);

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(ret));
    return MRS_FAIL;
  }

  return MRS_OK;
}

system_err_t generate_204_handler(httpd_req_t *req) {
  if (req == NULL) {
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  
  httpd_resp_set_status(req, "204 No Content");
  esp_err_t ret = httpd_resp_send(req, NULL, 0);
  
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send response: %s", esp_err_to_name(ret));
    return MRS_FAIL;
  }
  
  return MRS_OK;
}

system_err_t common_get_handler(httpd_req_t *req, httpd_err_code_t error) {
  if (req == NULL) {
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  
  ESP_LOGI(TAG, "Handling URI: %s", req->uri);
  return captive_portal_handler(req);
}

system_err_t captive_portal_handler(httpd_req_t *req) {
  if (req == NULL) {
    return MRS_ERR_CORE_INVALID_PARAM;
  }
  return root_handler(req);
}

system_err_t start_webserver(void) {
  if (server != NULL) {
    ESP_LOGW(TAG, "Web server already started");
    return MRS_OK;
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;
  config.max_uri_handlers = 10;

  esp_err_t ret = httpd_start(&server, &config);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Error starting web server: %s", esp_err_to_name(ret));
    return MRS_FAIL;
  }

  // Trang chủ
  httpd_uri_t root_uri = {.uri = "/",
                          .method = HTTP_GET,
                          .handler = root_handler,
                          .user_ctx = NULL};
  ret = httpd_register_uri_handler(server, &root_uri);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register root URI: %s", esp_err_to_name(ret));
    httpd_stop(server);
    server = NULL;
    return MRS_FAIL;
  }

  // Cấu hình WiFi
  httpd_uri_t config_uri = {.uri = "/configure",
                            .method = HTTP_POST,
                            .handler = wifi_config_handler,
                            .user_ctx = NULL};
  ret = httpd_register_uri_handler(server, &config_uri);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register config URI: %s", esp_err_to_name(ret));
    httpd_stop(server);
    server = NULL;
    return MRS_FAIL;
  }

  // Captive portal cho Android
  httpd_uri_t generate_204 = {.uri = "/generate_204",
                              .method = HTTP_GET,
                              .handler = generate_204_handler,
                              .user_ctx = NULL};
  ret = httpd_register_uri_handler(server, &generate_204);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register generate_204 URI: %s", esp_err_to_name(ret));
    httpd_stop(server);
    server = NULL;
    return MRS_FAIL;
  }

  // Captive portal cho iOS
  httpd_uri_t hotspot_detect = {.uri = "/hotspot-detect.html",
                                .method = HTTP_GET,
                                .handler = captive_portal_handler,
                                .user_ctx = NULL};
  ret = httpd_register_uri_handler(server, &hotspot_detect);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register hotspot-detect URI: %s", esp_err_to_name(ret));
    httpd_stop(server);
    server = NULL;
    return MRS_FAIL;
  }

  // Handler mặc định cho captive portal
  httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, common_get_handler);

  ESP_LOGI(TAG, "Web server started on port %d", config.server_port);
  return MRS_OK;
}

system_err_t stop_webserver(void) {
  if (server == NULL) {
    ESP_LOGW(TAG, "Web server not running");
    return MRS_OK;
  }

  esp_err_t ret = httpd_stop(server);
  server = NULL;
  
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to stop web server: %s", esp_err_to_name(ret));
    return MRS_FAIL;
  }

  ESP_LOGI(TAG, "Web server stopped");
  return MRS_OK;
}