#include "MqttManager.hpp"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_check.h"

#include <string>

static const char *TAG = "MQTT_MANAGER";

MqttManager::MqttManager(const MqttConfig &config)
    : config_(config),
      commandTopic_("shita/stations/" + std::string(config.stationId) + "/doors/command"),
      statusTopic_("shita/stations/" + std::string(config.stationId) + "/doors/response")
{
}

void MqttManager::setDoorCommandHandler(std::function<void(const DoorCommand &)> handler)
{
    doorCommandHandler_ = handler;
}

esp_err_t MqttManager::start()
{
    eventGroup_ = xEventGroupCreate();
    if (!eventGroup_)
    {
        return ESP_ERR_NO_MEM;
    }
    esp_mqtt_client_config_t mqttCfg = {};
    mqttCfg.broker.address.uri = config_.endpoint;
    mqttCfg.credentials.client_id = config_.clientId;
    mqttCfg.credentials.username = config_.username;
    mqttCfg.credentials.authentication.password = config_.password;

    client_ = esp_mqtt_client_init(&mqttCfg);
    if (!client_)
        return ESP_FAIL;

    ESP_RETURN_ON_ERROR(esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY, &MqttManager::eventHandler, this), TAG, "register event failed");
    ESP_RETURN_ON_ERROR(esp_mqtt_client_start(client_), TAG, "mqtt start failed");

    ESP_LOGI(TAG, "MQTT started: %s", config_.endpoint);
    return ESP_OK;
}

void MqttManager::setConnectedHandler(std::function<void()> handler)
{
    connectedHandler_ = handler;
}

esp_err_t MqttManager::waitConnected(TickType_t timeoutTicks) const
{
    if (!eventGroup_)
    {
        return ESP_ERR_INVALID_STATE;
    }

    EventBits_t bits = xEventGroupWaitBits(
        eventGroup_,
        MQTT_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        timeoutTicks);

    return (bits & MQTT_CONNECTED_BIT) ? ESP_OK : ESP_ERR_TIMEOUT;
}

bool MqttManager::isConnected() const
{
    return connected_;
}

esp_err_t MqttManager::publishReport(const CabinetReport &report)
{
    if (!client_ || !connected_)
    {
        ESP_LOGW(TAG, "MQTT not connected, report skipped: %s", report.message.c_str());
        return ESP_FAIL;
    }

    /*
     * IMPORTANT:
     * shita/stations/{stationId}/doors/response is consumed by the backend.
     * Therefore we publish ONLY the official backend contract:
     *
     * {
     *   "activityId": "...",
     *   "activity": "open|close|error",
     *   "doorId": "..."
     * }
     *
     * All debug/status/internal reports are logged locally only.
     */

    bool shouldPublishToBackend = false;
    std::string activity;

    if (report.type == CabinetReportType::DOOR_OPEN_SENT && report.success)
    {
        shouldPublishToBackend = true;
        activity = "open";
    }
    else if (report.type == CabinetReportType::DOOR_STATE_CHANGED &&
             !report.activity.empty() &&
             !report.activityId.empty() &&
             !report.doorId.empty())
    {
        shouldPublishToBackend = true;
        activity = report.activity;
    }
    else if (report.type == CabinetReportType::ERROR &&
             !report.activityId.empty() &&
             !report.doorId.empty())
    {
        shouldPublishToBackend = true;
        activity = "error";
    }

    if (!shouldPublishToBackend)
    {
        ESP_LOGI(TAG,
                 "Internal report only: type=%s success=%d doorId=%s activityId=%s message=%s",
                 reportTypeToString(report.type),
                 report.success,
                 report.doorId.c_str(),
                 report.activityId.c_str(),
                 report.message.c_str());

        return ESP_OK;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "activityId", report.activityId.c_str());
    cJSON_AddStringToObject(root, "activity", activity.c_str());
    cJSON_AddStringToObject(root, "doorId", report.doorId.c_str());

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json)
    {
        return ESP_ERR_NO_MEM;
    }

    int msgId = esp_mqtt_client_publish(
        client_,
        statusTopic_.c_str(),
        json,
        0,
        config_.qos,
        0);

    ESP_LOGI(TAG,
             "MQTT BACKEND RESPONSE topic=%s msg_id=%d payload=%s",
             statusTopic_.c_str(),
             msgId,
             json);

    cJSON_free(json);

    return msgId >= 0 ? ESP_OK : ESP_FAIL;
}

void MqttManager::eventHandler(void *handlerArgs, esp_event_base_t, int32_t, void *eventData)
{
    auto *self = static_cast<MqttManager *>(handlerArgs);
    self->handleEvent(static_cast<esp_mqtt_event_handle_t>(eventData));
}

void MqttManager::handleEvent(esp_mqtt_event_handle_t event)
{
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        connected_ = true;
        if (eventGroup_)
        {
            xEventGroupSetBits(eventGroup_, MQTT_CONNECTED_BIT);
        }
        ESP_LOGI(TAG, "MQTT connected, subscribing to %s", commandTopic_.c_str());
        esp_mqtt_client_subscribe(client_, commandTopic_.c_str(), config_.qos);
        publishReport(CabinetReport{CabinetReportType::BOOT, "", "", true, "mqtt_connected", "", ""});
        if (connectedHandler_)
        {
            connectedHandler_();
        }
        break;
    case MQTT_EVENT_DATA:
        handleMessage(event->topic, event->topic_len, event->data, event->data_len);
        break;
    case MQTT_EVENT_DISCONNECTED:
        connected_ = false;
        if (eventGroup_)
        {
            xEventGroupClearBits(eventGroup_, MQTT_CONNECTED_BIT);
        }
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    default:
        break;
    }
}

void MqttManager::handleMessage(const char *topic, int topicLen, const char *data, int dataLen)
{
    std::string topicStr(topic, topicLen);
    std::string payload(data, dataLen);
    ESP_LOGI(TAG, "MQTT RX topic=%s payload=%s", topicStr.c_str(), payload.c_str());

    cJSON *root = cJSON_ParseWithLength(data, dataLen);
    if (!root)
    {
        ESP_LOGE(TAG, "Invalid JSON payload");
        publishReport(CabinetReport{CabinetReportType::ERROR, "", "", false, "invalid_json", "", ""});
        return;
    }

    // cJSON *type = cJSON_GetObjectItem(root, "type");

    // if (cJSON_IsString(type) && std::string(type->valuestring) == "debug_reconnect")
    // {
    //     ESP_LOGW(TAG, "Debug reconnect simulation requested");

    //     if (connectedHandler_)
    //     {
    //         connectedHandler_();
    //     }

    //     cJSON_Delete(root);
    //     return;
    // }

    cJSON *doorId = cJSON_GetObjectItem(root, "doorId");
    cJSON *activityId = cJSON_GetObjectItem(root, "activityId");
    cJSON *time = cJSON_GetObjectItem(root, "time");

    if (!cJSON_IsString(doorId) || !cJSON_IsString(activityId))
    {
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

    if (doorCommandHandler_)
    {
        doorCommandHandler_(command);
    }

    cJSON_Delete(root);
}

const char *MqttManager::reportTypeToString(CabinetReportType type) const
{
    switch (type)
    {
    case CabinetReportType::BOOT:
        return "BOOT";
    case CabinetReportType::READY:
        return "READY";
    case CabinetReportType::COMMAND_RECEIVED:
        return "COMMAND_RECEIVED";
    case CabinetReportType::DOOR_OPEN_SENT:
        return "DOOR_OPEN_SENT";
    case CabinetReportType::DOOR_STATE_CHANGED:
        return "DOOR_STATE_CHANGED";
    case CabinetReportType::UART_RESPONSE:
        return "UART_RESPONSE";
    case CabinetReportType::ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}
