#include "ProtocolManager.hpp"

#include <cstdio>

std::string ProtocolManager::buildDiscoverManagement() const
{
    return "PFFF0e^0387.";
}

std::string ProtocolManager::buildFixManagementAddresses() const
{
    return buildFrame(0xFF, 0xF0, 'e', '_', {0x01});
}

std::string ProtocolManager::buildEnableShelfPower(uint8_t managementUnit) const
{
    return buildFrame(managementUnit, 0xF0, 'v', '=', {0x01});
}

std::string ProtocolManager::buildDiscoverShelfUnits(uint8_t managementUnit) const
{
    (void)managementUnit; // MVP document uses management 00.
    return "P00FFe^039D.";
}

std::string ProtocolManager::buildFixShelfAddresses(uint8_t managementUnit) const
{
    return buildFrame(managementUnit, 0xFF, 'e', '_', {0x02});
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
    return buildFrame(managementUnit, shelfUnit, 'i', '?', {0x00});
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

bool ProtocolManager::parseHexByte(const std::string& text, size_t index, uint8_t& value) const
{
    if (index + 1 >= text.size())
    {
        return false;
    }

    auto hexValue = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };

    int high = hexValue(text[index]);
    int low = hexValue(text[index + 1]);

    if (high < 0 || low < 0)
    {
        return false;
    }

    value = static_cast<uint8_t>((high << 4) | low);
    return true;
}

bool ProtocolManager::parseFrame(const std::string& frame, ProtocolMessage& message) const
{
    if (frame.size() < 10)
    {
        return false;
    }

    if (frame[0] != 'p')
    {
        return false;
    }

    if (frame.back() != '.')
    {
        return false;
    }

    uint8_t managementUnit = 0;
    uint8_t shelfUnit = 0;
    uint8_t remainingLength = 0;
    uint8_t receivedChecksum = 0;

    if (!parseHexByte(frame, 1, managementUnit)) return false;
    if (!parseHexByte(frame, 3, shelfUnit)) return false;

    char command = frame[5];
    char subCommand = frame[6];

    if (!parseHexByte(frame, 7, remainingLength)) return false;

    size_t expectedTotalLength = 1 + 2 + 2 + 1 + 1 + 2 + remainingLength;

    if (frame.size() != expectedTotalLength)
    {
        return false;
    }

    size_t checksumIndex = frame.size() - 3;

    if (!parseHexByte(frame, checksumIndex, receivedChecksum))
    {
        return false;
    }

    std::string frameWithoutChecksum = frame.substr(0, checksumIndex);
    uint8_t calculatedChecksum = calculateChecksum(frameWithoutChecksum);

    if (receivedChecksum != calculatedChecksum)
    {
        return false;
    }

    std::vector<uint8_t> data;

    size_t dataStart = 9;
    size_t dataEnd = checksumIndex;

    for (size_t i = dataStart; i < dataEnd; i += 2)
    {
        uint8_t byte = 0;

        if (!parseHexByte(frame, i, byte))
        {
            return false;
        }

        data.push_back(byte);
    }

    message.managementUnit = managementUnit;
    message.shelfUnit = shelfUnit;
    message.command = command;
    message.subCommand = subCommand;
    message.data = data;

    return true;
}

std::string ProtocolManager::buildErrorRead(uint8_t managementUnit, uint8_t shelfUnit) const
{
    return buildFrame(managementUnit, shelfUnit, '!', '?', {});
}

std::string ProtocolManager::buildClearErrors(uint8_t managementUnit, uint8_t shelfUnit) const
{
    return buildFrame(managementUnit, shelfUnit, '!', '_', {});
}
