#include "CabinetManager.hpp"

#include "esp_log.h"
#include "esp_check.h"

static const char* TAG = "CABINET_MANAGER";

CabinetManager::CabinetManager(const CabinetConfig& config,
                               ProtocolManager& protocol,
                               UartManager& uart)
    : config_(config), protocol_(protocol), uart_(uart) {}

void CabinetManager::setReportHandler(std::function<void(const CabinetReport&)> handler)
{
    reportHandler_ = handler;
}

esp_err_t CabinetManager::initializeHardware()
{
    ESP_LOGI(TAG, "Starting cabinet initialization sequence");
    state_ = CabinetState::DISCOVERING_UNITS;

    const std::string frames[] = {
        protocol_.buildDiscoverManagement(),
        protocol_.buildFixManagementAddresses(),
        protocol_.buildEnableShelfPower(),
        protocol_.buildDiscoverShelfUnits(0x00),
        protocol_.buildFixShelfAddresses(0x00),
    };

    for (const auto& frame : frames) {
        std::string response;
        esp_err_t err = sendAndRead(frame, &response);
        report(CabinetReport{CabinetReportType::UART_RESPONSE, "", "", err == ESP_OK,
                             err == ESP_OK ? "init_frame_ok" : "init_frame_failed", frame, response});
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Initialization failed on frame: %s", frame.c_str());
            state_ = CabinetState::ERROR;
            report(CabinetReport{CabinetReportType::ERROR, "", "", false, "cabinet_initialization_failed", frame, response});
            return err;
        }
    }

    state_ = CabinetState::READY;
    ESP_LOGI(TAG, "Cabinet hardware ready");
    report(CabinetReport{CabinetReportType::READY, "", "", true, "cabinet_ready", "", ""});
    return ESP_OK;
}

void CabinetManager::handleDoorCommand(const DoorCommand& command)
{
    ESP_LOGI(TAG, "Door command: doorId=%s activityId=%s", command.doorId.c_str(), command.activityId.c_str());

    if (state_ != CabinetState::READY) {
        ESP_LOGW(TAG, "Ignoring door command while cabinet is not ready");
        report(CabinetReport{CabinetReportType::ERROR, command.doorId, command.activityId, false,
                             "cabinet_not_ready", "", ""});
        return;
    }

    state_ = CabinetState::OPENING_DOOR;

    DoorAddress address = resolveDoorAddress(command.doorId);
    std::string frame = protocol_.buildOutputPulse(address.managementUnit,
                                                   address.shelfUnit,
                                                   address.output,
                                                   config_.defaultPulseMs);

    std::string response;
    esp_err_t err = sendAndRead(frame, &response);
    bool ok = (err == ESP_OK);

    report(CabinetReport{CabinetReportType::DOOR_OPEN_SENT, command.doorId, command.activityId, ok,
                         ok ? "door_open_command_sent" : "uart_error", frame, response});

    state_ = ok ? CabinetState::READY : CabinetState::ERROR;
}

DoorAddress CabinetManager::resolveDoorAddress(const std::string& doorId) const
{
    // TODO: Replace with real mapping from backend door configuration.
    // Safe MVP default: management=00, shelf=00, output=1.
    (void)doorId;
    return DoorAddress{0x00, 0x00, 0x01};
}

esp_err_t CabinetManager::sendAndRead(const std::string& frame, std::string* response)
{
    ESP_LOGI(TAG, "TX: %s", frame.c_str());
    ESP_RETURN_ON_ERROR(uart_.writeLine(frame), TAG, "UART write failed");

    std::string localResponse;
    std::string& target = response ? *response : localResponse;
    esp_err_t err = uart_.readFrame(target);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "RX: %s", target.c_str());
    } else {
        ESP_LOGW(TAG, "No response / timeout for frame: %s", frame.c_str());
    }
    return err;
}

void CabinetManager::report(const CabinetReport& reportData)
{
    if (reportHandler_) {
        reportHandler_(reportData);
    }
}
