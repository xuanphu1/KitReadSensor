
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#include "esp_mac.h"
#include "esp_bridge.h"
#include "esp_mesh_lite.h"
#include "Common.h"
#include "SensorRegistry.h"

// Forward declare để tránh phụ thuộc chéo nặng ở header
struct DataManager_t;

void MeshManager_StartMeshClient(struct DataManager_t *data);
bool MeshManager_IsConnected(void);