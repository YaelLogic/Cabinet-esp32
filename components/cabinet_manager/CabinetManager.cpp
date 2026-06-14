#include "CabinetManager.hpp"

#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "CABINET_MANAGER";

CabinetManager::CabinetManager(const CabinetConfig &config,
                               ProtocolManager &protocol,
                               UartManager &uart)
    : config_(config), protocol_(protocol), uart_(uart) {}

void CabinetManager::setReportHandler(std::function<void(const CabinetReport &)> handler)
{
    reportHandler_ = handler;
}

esp_err_t CabinetManager::initializeHardware()
{

    ESP_LOGI(TAG, "Starting cabinet initialization sequence");
    state_ = CabinetState::DISCOVERING_UNITS;

    struct InitFrame
    {
        std::string frame;
        bool expectResponse;
    };

    const InitFrame frames[] = {
        {protocol_.buildDiscoverManagement(), true},
        {protocol_.buildFixManagementAddresses(), false},
        {protocol_.buildEnableShelfPower(), false},
        {protocol_.buildDiscoverShelfUnits(0x00), true},
        {protocol_.buildFixShelfAddresses(0x00), false},
    };

    for (const auto &item : frames)
    {
        std::string response;
        esp_err_t err = sendAndRead(item.frame, &response, item.expectResponse);

        const bool ok = (err == ESP_OK);
        if (item.expectResponse && ok)
        {
            ProtocolMessage message;

            if (protocol_.parseFrame(response, message))
            {
                if (message.command == 'e' && message.subCommand == '^')
                {
                    if (message.shelfUnit == 0xF0)
                    {
                        managementCount_ = message.managementUnit + 1;
                        ESP_LOGI(TAG, "Detected management units: %u", managementCount_);
                    }
                    else
                    {
                        shelfCount_ = message.shelfUnit + 1;
                        ESP_LOGI(TAG, "Detected shelf units for M00: %u", shelfCount_);
                    }
                }
            }
        }

        report(CabinetReport{
            CabinetReportType::UART_RESPONSE,
            "",
            "",
            ok,
            ok ? "init_frame_ok" : "init_frame_failed",
            item.frame,
            response});

        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Initialization failed on frame: %s", item.frame.c_str());
            state_ = CabinetState::ERROR;

            report(CabinetReport{
                CabinetReportType::ERROR,
                "",
                "",
                false,
                "cabinet_initialization_failed",
                item.frame,
                response});

            return err;
        }
    }

    state_ = CabinetState::READY;
    ESP_LOGI(TAG, "Cabinet hardware ready");

    report(CabinetReport{
        CabinetReportType::READY,
        "",
        "",
        true,
        "cabinet_ready",
        "",
        ""});

    return ESP_OK;
}

void CabinetManager::handleDoorCommand(const DoorCommand &command)
{
    ESP_LOGI(TAG, "Door command: doorId=%s activityId=%s", command.doorId.c_str(), command.activityId.c_str());

    if (state_ != CabinetState::READY)
    {
        ESP_LOGW(TAG, "Ignoring door command while cabinet is not ready");
        report(CabinetReport{CabinetReportType::ERROR, command.doorId, command.activityId, false,
                             "cabinet_not_ready", "", ""});
        return;
    }

    state_ = CabinetState::OPENING_DOOR;

    DoorAddress address = resolveDoorAddress(command.doorId);
    if (address.managementUnit == 0xFF ||
        address.shelfUnit == 0xFF ||
        address.output == 0xFF)
    {

        ESP_LOGE(TAG, "Unknown doorId: %s", command.doorId.c_str());

        report(CabinetReport{
            CabinetReportType::ERROR,
            command.doorId,
            command.activityId,
            false,
            "unknown_door_id",
            "",
            ""});

        state_ = CabinetState::READY;
        return;
    }
    std::string frame = protocol_.buildOutputPulse(address.managementUnit,
                                                   address.shelfUnit,
                                                   address.output,
                                                   config_.defaultPulseMs);
    if (frame.empty())
    {
        ESP_LOGE(TAG, "Failed to build protocol frame for doorId=%s", command.doorId.c_str());

        report(CabinetReport{
            CabinetReportType::ERROR,
            command.doorId,
            command.activityId,
            false,
            "failed_to_build_protocol_frame",
            "",
            ""});

        state_ = CabinetState::READY;
        return;
    }
    std::string response;
    esp_err_t err = sendAndRead(frame, &response, false);
    bool ok = (err == ESP_OK);

    report(CabinetReport{CabinetReportType::DOOR_OPEN_SENT, command.doorId, command.activityId, ok,
                         ok ? "door_open_command_sent" : "uart_write_failed", frame, response});

    state_ = CabinetState::READY;
}

DoorAddress CabinetManager::resolveDoorAddress(const std::string &doorId) const
{
    const std::string prefix = std::string(config_.stationId) + "-";
    if (doorId.rfind(prefix, 0) != 0)
    {
        return DoorAddress{0xFF, 0xFF, 0xFF};
    }

    std::string doorNumberText = doorId.substr(prefix.size());

    if (doorNumberText.empty())
    {
        return DoorAddress{0xFF, 0xFF, 0xFF};
    }

    int doorNumber = 0;

    for (char c : doorNumberText)
    {
        if (c < '0' || c > '9')
        {
            return DoorAddress{0xFF, 0xFF, 0xFF};
        }

        doorNumber = doorNumber * 10 + (c - '0');
    }

    if (doorNumber <= 0)
    {
        return DoorAddress{0xFF, 0xFF, 0xFF};
    }

    const int maxDoorCount = static_cast<int>(shelfCount_) * 2;

    if (doorNumber > maxDoorCount)
    {
        ESP_LOGW(TAG,
                 "doorId %s out of range. shelfCount=%u maxDoorCount=%d",
                 doorId.c_str(),
                 shelfCount_,
                 maxDoorCount);

        return DoorAddress{0xFF, 0xFF, 0xFF};
    }

    int zeroBasedDoor = doorNumber - 1;

    uint8_t shelfUnit = static_cast<uint8_t>(zeroBasedDoor / 2);
    uint8_t output = static_cast<uint8_t>((zeroBasedDoor % 2) + 1);

    return DoorAddress{0x00, shelfUnit, output};
}

esp_err_t CabinetManager::sendAndRead(const std::string &frame,
                                      std::string *response,
                                      bool expectResponse)
{
    ESP_LOGI(TAG, "TX: %s", frame.c_str());
    ESP_RETURN_ON_ERROR(uart_.writeLine(frame), TAG, "UART write failed");

    if (!expectResponse)
    {
        ESP_LOGI(TAG, "No response expected for frame: %s", frame.c_str());
        return ESP_OK;
    }

    std::string localResponse;
    std::string &target = response ? *response : localResponse;

    esp_err_t err = uart_.readFrame(target);
    if (err == ESP_OK && target.empty())
    {
        ESP_LOGW(TAG, "Empty UART response for frame: %s", frame.c_str());
        return ESP_ERR_TIMEOUT;
    }

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "RX: %s", target.c_str());

        ProtocolMessage message;

        if (protocol_.parseFrame(target, message))
        {
            ESP_LOGI(TAG,
                     "PARSED: M=%02X SU=%02X CMD=%c SUB=%c DATA=%u",
                     message.managementUnit,
                     message.shelfUnit,
                     message.command,
                     message.subCommand,
                     static_cast<unsigned>(message.data.size()));
        }
        else
        {
            ESP_LOGW(TAG, "Failed to parse response: %s", target.c_str());
        }
    }
    else
    {
        ESP_LOGW(TAG, "No response / timeout for frame: %s", frame.c_str());
    }

    return err;
}

void CabinetManager::report(const CabinetReport &reportData)
{
    if (reportHandler_)
    {
        reportHandler_(reportData);
    }
}
