#include "UartManager.hpp"

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_check.h"

static const char* TAG = "UART_MANAGER";

UartManager::UartManager(const UartConfig& config) : config_(config) {}

esp_err_t UartManager::start()
{
    uart_config_t uartConfig = {};
    uartConfig.baud_rate = config_.baudRate;
    uartConfig.data_bits = UART_DATA_8_BITS;
    uartConfig.parity = UART_PARITY_DISABLE;
    uartConfig.stop_bits = UART_STOP_BITS_1;
    uartConfig.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uartConfig.source_clk = UART_SCLK_DEFAULT;

    ESP_RETURN_ON_ERROR(uart_driver_install(config_.port, config_.rxBufferSize, config_.txBufferSize, 0, nullptr, 0), TAG, "driver install failed");
    ESP_RETURN_ON_ERROR(uart_param_config(config_.port, &uartConfig), TAG, "param config failed");
    ESP_RETURN_ON_ERROR(uart_set_pin(config_.port, config_.txPin, config_.rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE), TAG, "set pin failed");

    ESP_LOGI(TAG, "UART started: port=%d baud=%d", config_.port, config_.baudRate);
    return ESP_OK;
}

esp_err_t UartManager::writeLine(const std::string& frame)
{
    if (frame.empty()) return ESP_ERR_INVALID_ARG;
    int written = uart_write_bytes(config_.port, frame.data(), frame.size());
    return written == static_cast<int>(frame.size()) ? ESP_OK : ESP_FAIL;
}

esp_err_t UartManager::readFrame(std::string& outFrame)
{
    uint8_t buffer[256] = {0};
    int len = uart_read_bytes(config_.port, buffer, sizeof(buffer) - 1, pdMS_TO_TICKS(config_.readTimeoutMs));
    if (len <= 0) return ESP_ERR_TIMEOUT;

    outFrame.assign(reinterpret_cast<char*>(buffer), len);
    return ESP_OK;
}
