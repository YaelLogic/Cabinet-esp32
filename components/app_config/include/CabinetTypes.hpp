#pragma once

#include <cstdint>
#include <string>

struct DoorCommand {
    std::string doorId;      // Backend door id
    std::string activityId;  // Backend activity id
    std::string time;        // ISO timestamp from backend
};

struct DoorAddress {
    uint8_t managementUnit = 0;
    uint8_t shelfUnit = 0;
    uint8_t output = 1;
};

enum class CabinetState {
    BOOT,
    DISCOVERING_UNITS,
    READY,
    OPENING_DOOR,
    ERROR
};

enum class CabinetReportType {
    BOOT,
    READY,
    COMMAND_RECEIVED,
    DOOR_OPEN_SENT,
    UART_RESPONSE,
    ERROR
};

struct CabinetReport {
    CabinetReportType type = CabinetReportType::BOOT;
    std::string doorId;
    std::string activityId;
    bool success = false;
    std::string message;
    std::string uartFrame;
    std::string uartResponse;
    int doorCount = -1;
};
