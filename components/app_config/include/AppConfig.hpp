#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"

struct WifiConfig {
    const char* ssid = "Hamazon";
    const char* password = "0504141078";
};

struct MqttConfig {
    const char* endpoint = "mqtt://192.168.1.4:1883";
    const char* stationId = "03";
    const char* clientId = "cabinet-03";
    const char* username = "";
    const char* password = "secret-token-for-door-esp32-03:YOUR_SECRET";

    // Incoming backend command topic:
    // shita/stations/{stationId}/doors/command
    const char* commandTopic = "shita/stations/03/doors/command";

    // Outgoing ESP reports topic:
    // shita/stations/{stationId}/doors/status
    const char* statusTopic = "shita/stations/03/doors/status";

    int qos = 1;
};

struct UartConfig {
    uart_port_t port = UART_NUM_1;
    gpio_num_t txPin = GPIO_NUM_17;
    gpio_num_t rxPin = GPIO_NUM_18;
    int baudRate = 9600;
    int rxBufferSize = 1024;
    int txBufferSize = 1024;
    int readTimeoutMs = 500;
};

struct CabinetConfig {
    int defaultPulseMs = 500;
};

struct AppConfig {
    WifiConfig wifi;
    MqttConfig mqtt;
    UartConfig uart;
    CabinetConfig cabinet;
};
