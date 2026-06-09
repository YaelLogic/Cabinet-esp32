#include "MqttManager.hpp"

#include "cJSON.h"
#include "esp_log.h"
#include "esp_check.h"

#include <string>

static const char* TAG = "MQTT_MANAGER";

MqttManager::MqttManager(const MqttConfig& config) : config_(config) {}

void MqttManager::setDoorCommandHandler(std::function<void(const DoorCommand&)> handler)
{
    doorCommandHandler_ = handler;
}

esp_err_t MqttManager::start()
{
    esp_mqtt_client_config_t mqttCfg = {};
    mqttCfg.broker.address.uri = config_.endpoint;
    mqttCfg.credentials.client_id = config_.clientId;
    mqttCfg.credentials.username = config_.username;
    mqttCfg.credentials.authentication.password = config_.password;

    client_ = esp_mqtt_client_init(&mqttCfg);
    if (!client_) return ESP_FAIL;

    ESP_RETURN_ON_ERROR(esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY, &MqttManager::eventHandler, this), TAG, "register event failed");
    ESP_RETURN_ON_ERROR(esp_mqtt_client_start(client_), TAG, "mqtt start failed");

    ESP_LOGI(TAG, "MQTT started: %s", config_.endpoint);
    return ESP_OK;
}

esp_err_t MqttManager::publishReport(const CabinetReport& report)
{
    if (!client_ || !connected_) {
        ESP_LOGW(TAG, "MQTT not connected, report skipped: %s", report.message.c_str());
        return ESP_FAIL;
    }

    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "stationId", config_.stationId);
    cJSON_AddStringToObject(root, "type", reportTypeToString(report.type));
    cJSON_AddBoolToObject(root, "success", report.success);
    cJSON_AddStringToObject(root, "message", report.message.c_str());

    if (!report.doorId.empty()) cJSON_AddStringToObject(root, "doorId", report.doorId.c_str());
    if (!report.activityId.empty()) cJSON_AddStringToObject(root, "activityId", report.activityId.c_str());
    if (!report.uartFrame.empty()) cJSON_AddStringToObject(root, "uartFrame", report.uartFrame.c_str());
    if (!report.uartResponse.empty()) cJSON_AddStringToObject(root, "uartResponse", report.uartResponse.c_str());

    char* json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return ESP_ERR_NO_MEM;

    int msgId = esp_mqtt_client_publish(client_, config_.statusTopic, json, 0, config_.qos, 0);
    ESP_LOGI(TAG, "MQTT TX topic=%s msg_id=%d payload=%s", config_.statusTopic, msgId, json);
    cJSON_free(json);

    return msgId >= 0 ? ESP_OK : ESP_FAIL;
}

void MqttManager::eventHandler(void* handlerArgs, esp_event_base_t, int32_t, void* eventData)
{
    auto* self = static_cast<MqttManager*>(handlerArgs);
    self->handleEvent(static_cast<esp_mqtt_event_handle_t>(eventData));
}

void MqttManager::handleEvent(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            connected_ = true;
            ESP_LOGI(TAG, "MQTT connected, subscribing to %s", config_.commandTopic);
            esp_mqtt_client_subscribe(client_, config_.commandTopic, config_.qos);
            publishReport(CabinetReport{CabinetReportType::BOOT, "", "", true, "mqtt_connected", "", ""});
            break;
        case MQTT_EVENT_DATA:
            handleMessage(event->topic, event->topic_len, event->data, event->data_len);
            break;
        case MQTT_EVENT_DISCONNECTED:
            connected_ = false;
            ESP_LOGW(TAG, "MQTT disconnected");
            break;
        default:
            break;
    }
}

void MqttManager::handleMessage(const char* topic, int topicLen, const char* data, int dataLen)
{
    std::string topicStr(topic, topicLen);
    std::string payload(data, dataLen);
    ESP_LOGI(TAG, "MQTT RX topic=%s payload=%s", topicStr.c_str(), payload.c_str());

    cJSON* root = cJSON_ParseWithLength(data, dataLen);
    if (!root) {
        ESP_LOGE(TAG, "Invalid JSON payload");
        publishReport(CabinetReport{CabinetReportType::ERROR, "", "", false, "invalid_json", "", ""});
        return;
    }

    cJSON* doorId = cJSON_GetObjectItem(root, "doorId");
    cJSON* activityId = cJSON_GetObjectItem(root, "activityId");
    cJSON* time = cJSON_GetObjectItem(root, "time");

    if (!cJSON_IsString(doorId) || !cJSON_IsString(activityId)) {
        ESP_LOGE(TAG, "Missing doorId/activityId");
        publishReport(CabinetReport{CabinetReportType::ERROR, "", "", false, "missing_doorId_or_activityId", "", ""});
        cJSON_Delete(root);
        return;
    }

    DoorCommand command;
    command.doorId = doorId->valuestring;
    command.activityId = activityId->valuestring;
    command.time = cJSON_IsString(time) ? time->valuestring : "";

    publishReport(CabinetReport{CabinetReportType::COMMAND_RECEIVED, command.doorId, command.activityId, true, "command_received", "", ""});

    if (doorCommandHandler_) {
        doorCommandHandler_(command);
    }

    cJSON_Delete(root);
}

const char* MqttManager::reportTypeToString(CabinetReportType type) const
{
    switch (type) {
        case CabinetReportType::BOOT: return "BOOT";
        case CabinetReportType::READY: return "READY";
        case CabinetReportType::COMMAND_RECEIVED: return "COMMAND_RECEIVED";
        case CabinetReportType::DOOR_OPEN_SENT: return "DOOR_OPEN_SENT";
        case CabinetReportType::UART_RESPONSE: return "UART_RESPONSE";
        case CabinetReportType::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
