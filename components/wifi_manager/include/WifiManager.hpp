#pragma once

#include "AppConfig.hpp"
#include "esp_err.h"

class WifiManager {
public:
    explicit WifiManager(const WifiConfig& config);
    esp_err_t start();

private:
    WifiConfig config_;
};
