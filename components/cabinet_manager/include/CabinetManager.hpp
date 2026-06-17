#pragma once

#include "AppConfig.hpp"
#include "CabinetTypes.hpp"
#include "ProtocolManager.hpp"
#include "UartManager.hpp"

#include "freertos/task.h"
#include "freertos/semphr.h"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>


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
    struct DoorEntry
    {
        std::string doorId;
        DoorAddress address;
    };

    struct PendingOpenActivity
    {
        bool active = false;
        std::string doorId;
        std::string activityId;
        int64_t startedAtUs = 0;
    };

    DoorAddress resolveDoorAddress(const std::string &doorId) const;
    esp_err_t sendAndRead(const std::string &frame,
                          std::string *response = nullptr,
                          bool expectResponse = true);
    void report(const CabinetReport &report);

    void buildDoorMap();

    static void sensorTaskEntry(void *arg);
    void sensorMonitorLoop();
    void pollShelfInputs();
    void handleInputValue(uint8_t shelfUnit, uint8_t value);
    void checkPendingOpenTimeouts();

    const char *doorStateToString(DoorState state) const;

    CabinetConfig config_;
    ProtocolManager &protocol_;
    UartManager &uart_;
    std::function<void(const CabinetReport &)> reportHandler_;
    CabinetState state_ = CabinetState::BOOT;

    uint8_t managementCount_ = 0;
    uint8_t shelfCount_ = 0;

    std::vector<DoorEntry> doors_;

    SemaphoreHandle_t uartMutex_ = nullptr;
    TaskHandle_t sensorTaskHandle_ = nullptr;

    std::array<DoorState, 16> lastDoorStates_{};
    std::array<PendingOpenActivity, 16> pendingOpen_{};

    static constexpr int64_t OPEN_CONFIRM_TIMEOUT_US = 15 * 1000 * 1000;

};
