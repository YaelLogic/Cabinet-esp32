#include "AppConfig.hpp"
#include "WifiManager.hpp"
#include "MqttManager.hpp"
#include "UartManager.hpp"
#include "ProtocolManager.hpp"
#include "CabinetManager.hpp"

#include "esp_log.h"
#include "nvs_flash.h"

static const char* TAG = "CABINET_APP";

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    AppConfig config;

    ESP_LOGI(TAG, "Cabinet gateway boot");

    WifiManager wifi(config.wifi);
    ESP_ERROR_CHECK(wifi.start());

    MqttManager mqtt(config.mqtt);
    ESP_ERROR_CHECK(mqtt.start());

    UartManager uart(config.uart);
    ESP_ERROR_CHECK(uart.start());

    ProtocolManager protocol;
    CabinetManager cabinet(config.cabinet, protocol, uart);

    cabinet.setReportHandler([&mqtt](const CabinetReport& report) {
        mqtt.publishReport(report);
    });

    ESP_ERROR_CHECK(cabinet.initializeHardware());

    mqtt.setDoorCommandHandler([&cabinet](const DoorCommand& command) {
        cabinet.handleDoorCommand(command);
    });

    ESP_LOGI(TAG, "System ready");
}
