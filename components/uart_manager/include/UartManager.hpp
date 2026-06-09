#pragma once

#include "AppConfig.hpp"
#include "esp_err.h"

#include <string>

class UartManager {
public:
    explicit UartManager(const UartConfig& config);

    esp_err_t start();
    esp_err_t writeLine(const std::string& frame);
    esp_err_t readFrame(std::string& outFrame);

private:
    UartConfig config_;
};
