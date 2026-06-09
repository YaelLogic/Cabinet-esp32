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

std::string ProtocolManager::buildOutputPulse(uint8_t managementUnit,
                                              uint8_t shelfUnit,
                                              uint8_t outputNumber,
                                              int pulseMs) const
{
    (void)pulseMs; // Document examples are 0.5 sec pulse.

    // Known-good MVP frames from protocol document.
    if (managementUnit == 0x00 && shelfUnit == 0x00 && outputNumber == 0x01) return "P0000o=07010516.";
    if (managementUnit == 0x00 && shelfUnit == 0x01 && outputNumber == 0x01) return "P0001o=07010515.";
    if (managementUnit == 0x00 && shelfUnit == 0x00 && outputNumber == 0x02) return "P0000o=07020515.";
    if (managementUnit == 0x00 && shelfUnit == 0x01 && outputNumber == 0x02) return "P0001o=07020514.";

    // TODO: Implement generic checksum builder when final checksum formula is confirmed.
    return "";
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
