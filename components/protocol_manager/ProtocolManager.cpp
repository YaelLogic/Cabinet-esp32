#include "ProtocolManager.hpp"

#include <cstdio>

std::string ProtocolManager::buildDiscoverManagement() const
{
    return "PFFF0e^0387.";
}

std::string ProtocolManager::buildFixManagementAddresses() const
{
    return "PFFF0e_050123.";
}

std::string ProtocolManager::buildEnableShelfPower(uint8_t managementUnit) const
{
    (void)managementUnit; // MVP document uses management 00.
    return "P00F0v=050160.";
}

std::string ProtocolManager::buildDiscoverShelfUnits(uint8_t managementUnit) const
{
    (void)managementUnit; // MVP document uses management 00.
    return "P00FFe^039D.";
}

std::string ProtocolManager::buildFixShelfAddresses(uint8_t managementUnit) const
{
    (void)managementUnit; // MVP document uses management 00.
    return "P00FFe_050234.";
}

std::string ProtocolManager::buildFrame(uint8_t managementUnit,
                                        uint8_t shelfUnit,
                                        char command,
                                        char subCommand,
                                        const std::vector<uint8_t>& data) const
{
    std::string frame;

    frame += 'P';
    frame += hex2(managementUnit);
    frame += hex2(shelfUnit);
    frame += command;
    frame += subCommand;

    uint8_t remainingLength = static_cast<uint8_t>((data.size() * 2) + 3);
    frame += hex2(remainingLength);

    for (uint8_t byte : data)
    {
        frame += hex2(byte);
    }

    uint8_t checksum = calculateChecksum(frame);
    frame += hex2(checksum);
    frame += '.';

    return frame;
}

uint8_t ProtocolManager::calculateChecksum(const std::string& frameWithoutChecksum) const
{
    uint32_t sum = 0;

    for (char c : frameWithoutChecksum)
    {
        sum += static_cast<uint8_t>(c);
    }

    return static_cast<uint8_t>(255 - (sum % 256));
}
std::string ProtocolManager::buildOutputPulse(uint8_t managementUnit,
                                              uint8_t shelfUnit,
                                              uint8_t outputNumber,
                                              int pulseMs) const
{
    uint8_t pulseTenths = static_cast<uint8_t>(pulseMs / 100);

    if (pulseTenths == 0)
    {
        pulseTenths = 1;
    }

    return buildFrame(
        managementUnit,
        shelfUnit,
        'o',
        '=',
        {outputNumber, pulseTenths}
    );
}

std::string ProtocolManager::buildLedFlash(uint8_t managementUnit, uint8_t shelfUnit, bool enabled) const
{
    if (managementUnit == 0x00 && shelfUnit == 0x00 && enabled) return "P0000f=050116.";
    if (managementUnit == 0x00 && shelfUnit == 0x00 && !enabled) return "P0000f=050017.";
    if (managementUnit == 0x00 && shelfUnit == 0x01 && enabled) return "P0001f=050115.";
    if (managementUnit == 0x00 && shelfUnit == 0x01 && !enabled) return "P0001f=050016.";
    return "";
}

std::string ProtocolManager::buildInputRead(uint8_t managementUnit, uint8_t shelfUnit) const
{
    if (managementUnit == 0x00 && shelfUnit == 0x00) return "P0000i?0400E8.";
    if (managementUnit == 0x00 && shelfUnit == 0x01) return "P0001i?0400E7.";
    return "";
}

std::string ProtocolManager::buildFirmwareRead(uint8_t managementUnit, uint8_t shelfUnit) const
{
    if (managementUnit == 0x00 && shelfUnit == 0x0F) return "P000F#?03B8.";
    if (managementUnit == 0x00 && shelfUnit == 0x00) return "P0000#?03C7.";
    if (managementUnit == 0x00 && shelfUnit == 0x01) return "P0001#?03C6.";
    return "";
}

std::string ProtocolManager::hex2(uint8_t value)
{
    char buf[3];
    std::snprintf(buf, sizeof(buf), "%02X", value);
    return std::string(buf);
}