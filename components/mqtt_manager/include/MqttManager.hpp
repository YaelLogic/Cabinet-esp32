#pragma once

#include "AppConfig.hpp"
#include "CabinetTypes.hpp"
#include "esp_err.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <functional>

class MqttManager
{
public:
    explicit MqttManager(const MqttConfig &config);

    void setDoorCommandHandler(std::function<void(const DoorCommand &)> handler);
    esp_err_t start();
    esp_err_t publishReport(const CabinetReport &report);
    esp_err_t waitConnected(TickType_t timeoutTicks = pdMS_TO_TICKS(15000)) const;
    bool isConnected() const;

private:
    static void eventHandler(void *handlerArgs, esp_event_base_t base, int32_t eventId, void *eventData);
    void handleEvent(esp_mqtt_event_handle_t event);
    void handleMessage(const char *topic, int topicLen, const char *data, int dataLen);
    const char *reportTypeToString(CabinetReportType type) const;

    MqttConfig config_;
    esp_mqtt_client_handle_t client_ = nullptr;
    bool connected_ = false;
    std::function<void(const DoorCommand &)> doorCommandHandler_;
    EventGroupHandle_t eventGroup_ = nullptr;
    static constexpr EventBits_t MQTT_CONNECTED_BIT = BIT0;
};
