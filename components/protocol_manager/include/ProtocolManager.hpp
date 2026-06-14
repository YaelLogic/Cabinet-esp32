#pragma once

#include <cstdint>
#include <string>
#include <vector>

class ProtocolManager
{
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
    std::string buildFrame(uint8_t managementAddress,
                           uint8_t shelfAddress,
                           char command,
                           char subCommand,
                           const std::vector<uint8_t>& data) const;

    uint8_t calculateChecksum(const std::string& frameWithoutChecksum) const;

    static std::string hex2(uint8_t value);
};