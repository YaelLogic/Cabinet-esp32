#include "WifiManager.hpp"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <cstring>

static const char* TAG = "WIFI_MANAGER";

WifiManager::WifiManager(const WifiConfig& config) : config_(config) {}

esp_err_t WifiManager::start()
{
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "event loop failed");
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init failed");

    wifi_config_t wifiConfig = {};
    std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.ssid), config_.ssid, sizeof(wifiConfig.sta.ssid));
    std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.password), config_.password, sizeof(wifiConfig.sta.password));

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "set mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig), TAG, "set config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");
    ESP_RETURN_ON_ERROR(esp_wifi_connect(), TAG, "wifi connect failed");

    ESP_LOGI(TAG, "WiFi connect requested for SSID=%s", config_.ssid);
    return ESP_OK;
}
