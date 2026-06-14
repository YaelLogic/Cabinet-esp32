#pragma once

#include "AppConfig.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

class WifiManager {
public:
    explicit WifiManager(const WifiConfig& config);

    esp_err_t start();
    esp_err_t waitConnected(TickType_t timeoutTicks = pdMS_TO_TICKS(15000));

private:
    static void eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);

    WifiConfig config_;
    EventGroupHandle_t eventGroup_ = nullptr;

    static constexpr int WIFI_CONNECTED_BIT = BIT0;
    static constexpr int WIFI_FAIL_BIT = BIT1;
};