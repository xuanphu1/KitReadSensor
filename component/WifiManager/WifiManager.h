#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include "DataManager.h"

// WiFi event group bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_AP_MODE_BIT BIT2

// WiFi configuration
#define AP_SSID CONFIG_AP_SSID
#define AP_PASSWORD CONFIG_AP_PASSWORD
#define AP_CHANNEL CONFIG_AP_CHANNEL
#define AP_MAX_CONN 4
#define DNS_PORT 53
#define DNS_MAX_LEN 512


// Function declarations
void wifi_init_ap(void);
esp_err_t wifi_init_common(void);
void wifi_init_sta(void *pvParameters);
void wifi_connect_task(void *pvParameters);
void wifi_manager_task(void *pvParameters);
void dns_server_task(void *pvParameters);
void wifi_connect_handler(DataManager_t *data);
void update_wifi_status(wifiInfo_t *WifiInfo);

// Web server functions
esp_err_t start_webserver(void);
esp_err_t stop_webserver(void);
esp_err_t init_spiffs(void);
esp_err_t root_handler(httpd_req_t *req);
esp_err_t wifi_config_handler(httpd_req_t *req);
esp_err_t wifi_status_handler(httpd_req_t *req);
esp_err_t redirect_handler(httpd_req_t *req);
esp_err_t generate_204_handler(httpd_req_t *req);
esp_err_t common_get_handler(httpd_req_t *req, httpd_err_code_t error);
esp_err_t captive_portal_handler(httpd_req_t *req);

// External variables
extern EventGroupHandle_t wifi_event_group;
extern bool ap_mode_active;
extern char pending_ssid[33];
extern char pending_password[65];
extern bool wifi_connect_pending;
extern bool wifi_initialized;
extern esp_netif_t *sta_netif;
extern esp_netif_t *ap_netif;
extern SemaphoreHandle_t wifi_mutex;
extern int retry_num;
extern httpd_handle_t server;
extern TaskHandle_t dns_server_task_handle;
#endif // __WIFI_MANAGER_H__