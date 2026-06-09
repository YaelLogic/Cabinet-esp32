#pragma once

#include "AppConfig.hpp"
#include "CabinetTypes.hpp"
#include "ProtocolManager.hpp"
#include "UartManager.hpp"

#include <functional>

class CabinetManager {
public:
    CabinetManager(const CabinetConfig& config,
                   ProtocolManager& protocol,
                   UartManager& uart);

    void setReportHandler(std::function<void(const CabinetReport&)> handler);
    esp_err_t initializeHardware();
    void handleDoorCommand(const DoorCommand& command);

private:
    DoorAddress resolveDoorAddress(const std::string& doorId) const;
    esp_err_t sendAndRead(const std::string& frame, std::string* response = nullptr);
    void report(const CabinetReport& report);

    CabinetConfig config_;
    ProtocolManager& protocol_;
    UartManager& uart_;
    std::function<void(const CabinetReport&)> reportHandler_;
    CabinetState state_ = CabinetState::BOOT;
};
