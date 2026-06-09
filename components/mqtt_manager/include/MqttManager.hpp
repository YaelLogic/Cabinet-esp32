#pragma once

#include "AppConfig.hpp"
#include "CabinetTypes.hpp"
#include "esp_err.h"
#include "mqtt_client.h"

#include <functional>

class MqttManager {
public:
    explicit MqttManager(const MqttConfig& config);

    void setDoorCommandHandler(std::function<void(const DoorCommand&)> handler);
    esp_err_t start();
    esp_err_t publishReport(const CabinetReport& report);

private:
    static void eventHandler(void* handlerArgs, esp_event_base_t base, int32_t eventId, void* eventData);
    void handleEvent(esp_mqtt_event_handle_t event);
    void handleMessage(const char* topic, int topicLen, const char* data, int dataLen);
    const char* reportTypeToString(CabinetReportType type) const;

    MqttConfig config_;
    esp_mqtt_client_handle_t client_ = nullptr;
    bool connected_ = false;
    std::function<void(const DoorCommand&)> doorCommandHandler_;
};
