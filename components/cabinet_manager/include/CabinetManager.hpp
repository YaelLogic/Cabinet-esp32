#pragma once

#include "AppConfig.hpp"
#include "CabinetTypes.hpp"
#include "ProtocolManager.hpp"
#include "UartManager.hpp"
#include <vector>
#include <functional>
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <array>

class CabinetManager
{
public:
    CabinetManager(const CabinetConfig &config,
                   ProtocolManager &protocol,
                   UartManager &uart);

    void setReportHandler(std::function<void(const CabinetReport &)> handler);
    esp_err_t initializeHardware();
    void handleDoorCommand(const DoorCommand &command);

    void publishReadyStatus();

private:
    DoorAddress resolveDoorAddress(const std::string &doorId) const;
    esp_err_t sendAndRead(const std::string &frame, std::string *response = nullptr, bool expectResponse = true);
    void report(const CabinetReport &report);

    CabinetConfig config_;
    ProtocolManager &protocol_;
    UartManager &uart_;
    std::function<void(const CabinetReport &)> reportHandler_;
    CabinetState state_ = CabinetState::BOOT;

    uint8_t managementCount_ = 0;
    uint8_t shelfCount_ = 0;

    struct DoorEntry
    {
        std::string doorId;
        DoorAddress address;
    };

    std::vector<DoorEntry> doors_;

    void buildDoorMap();

    static void sensorTaskEntry(void *arg);
    void sensorMonitorLoop();
    void pollShelfInputs();
    void handleInputValue(uint8_t shelfUnit, uint8_t value);

    DoorState decodeDoorState(uint8_t value, uint8_t bitIndex) const;
    const char *doorStateToString(DoorState state) const;

    SemaphoreHandle_t uartMutex_ = nullptr;
    TaskHandle_t sensorTaskHandle_ = nullptr;

    std::array<DoorState, 16> lastDoorStates_{};
};