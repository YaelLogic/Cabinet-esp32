#include "WifiManager.hpp"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"

#include <cstring>

static const char* TAG = "WIFI_MANAGER";

WifiManager::WifiManager(const WifiConfig& config) : config_(config) {}

esp_err_t WifiManager::start()
{
    eventGroup_ = xEventGroupCreate();
    if (!eventGroup_) {
        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init failed");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "event loop failed");

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init failed");

    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &WifiManager::eventHandler,
            this,
            nullptr
        ),
        TAG,
        "register WIFI event failed"
    );

    ESP_RETURN_ON_ERROR(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &WifiManager::eventHandler,
            this,
            nullptr
        ),
        TAG,
        "register IP event failed"
    );

    wifi_config_t wifiConfig = {};
    std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.ssid),
                 config_.ssid,
                 sizeof(wifiConfig.sta.ssid));

    std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.password),
                 config_.password,
                 sizeof(wifiConfig.sta.password));

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "set mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig), TAG, "set config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");
    ESP_RETURN_ON_ERROR(esp_wifi_connect(), TAG, "wifi connect failed");

    ESP_LOGI(TAG, "WiFi connect requested for SSID=%s", config_.ssid);
    return ESP_OK;
}

esp_err_t WifiManager::waitConnected(TickType_t timeoutTicks)
{
    if (!eventGroup_) {
        return ESP_ERR_INVALID_STATE;
    }

    EventBits_t bits = xEventGroupWaitBits(
        eventGroup_,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        timeoutTicks
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected");
        return ESP_OK;
    }

    if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "WiFi connection failed");
        return ESP_FAIL;
    }

    ESP_LOGE(TAG, "WiFi connection timeout");
    return ESP_ERR_TIMEOUT;
}

void WifiManager::eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData)
{
    auto* self = static_cast<WifiManager*>(arg);

    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting");
        esp_wifi_connect();
        xEventGroupClearBits(self->eventGroup_, WIFI_CONNECTED_BIT);
        return;
    }

    if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(eventData);

        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        xEventGroupSetBits(self->eventGroup_, WIFI_CONNECTED_BIT);
        return;
    }
}