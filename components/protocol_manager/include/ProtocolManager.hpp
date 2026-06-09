#pragma once

#include <cstdint>
#include <string>

class ProtocolManager {
public:
    std::string buildDiscoverManagement() const;
    std::string buildFixManagementAddresses() const;
    std::string buildEnableShelfPower(uint8_t managementUnit = 0x00) const;
    std::string buildDiscoverShelfUnits(uint8_t managementUnit) const;
    std::string buildFixShelfAddresses(uint8_t managementUnit) const;

    std::string buildOutputPulse(uint8_t managementUnit,
                                 uint8_t shelfUnit,
                                 uint8_t outputNumber,
                                 int pulseMs) const;

    std::string buildLedFlash(uint8_t managementUnit, uint8_t shelfUnit, bool enabled) const;
    std::string buildInputRead(uint8_t managementUnit, uint8_t shelfUnit) const;
    std::string buildFirmwareRead(uint8_t managementUnit, uint8_t shelfUnit) const;

private:
    static std::string hex2(uint8_t value);
};
