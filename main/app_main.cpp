#include "AppConfig.hpp"
#include "WifiManager.hpp"
#include "MqttManager.hpp"
#include "UartManager.hpp"
#include "ProtocolManager.hpp"
#include "CabinetManager.hpp"

#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "APP_MAIN";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Cabinet ESP32 starting");

    AppConfig config;

    ESP_ERROR_CHECK(nvs_flash_init());

    WifiManager wifi(config.wifi);
    ESP_ERROR_CHECK(wifi.start());
    ESP_ERROR_CHECK(wifi.waitConnected());

    UartManager uart(config.uart);
    ESP_ERROR_CHECK(uart.start());

    ProtocolManager protocol;
    CabinetManager cabinet(config.cabinet, protocol, uart);

    MqttManager mqtt(config.mqtt);

    mqtt.setDoorCommandHandler([&cabinet](const DoorCommand &command)
                               { cabinet.handleDoorCommand(command); });

    cabinet.setReportHandler([&mqtt](const CabinetReport &report)
                             { mqtt.publishReport(report); });

    ESP_ERROR_CHECK(mqtt.start());
    ESP_ERROR_CHECK(mqtt.waitConnected(pdMS_TO_TICKS(15000)));

    ESP_ERROR_CHECK(cabinet.initializeHardware());

    mqtt.setConnectedHandler([&cabinet]()
                         { cabinet.publishReadyStatus(); });
                         
    ESP_LOGI(TAG, "System ready");

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}