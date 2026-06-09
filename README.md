# Cabinet ESP32 Gateway

ESP-IDF project skeleton for the smart cabinet gateway.

The ESP32 replaces the old PC/local controller. It subscribes to backend door commands over MQTT, translates them to the existing cabinet serial protocol, sends them to the Management Unit over UART, and reports status back to the backend over MQTT.

## Current design decision

Status responses are returned over MQTT, not HTTPS.

This is the preferred design for the MVP because the ESP32 already keeps an MQTT connection open, and cabinet events are naturally asynchronous:

- command received
- UART frame sent
- UART response received
- door command completed
- timeout
- cabinet error
- cabinet ready

## First MVP

1. Connect Wi-Fi.
2. Connect MQTT.
3. Subscribe to `shita/stations/{stationId}/doors/command`.
4. Publish reports to `shita/stations/{stationId}/doors/status`.
5. Open UART to the Management Unit.
6. Run cabinet initialization sequence.
7. Receive `doorId` + `activityId` from MQTT.
8. Convert door address to protocol command.
9. Send pulse command over UART.
10. Publish result over MQTT.

## Build

```bash
idf.py set-target esp32s3
idf.py menuconfig
idf.py build
```

## Configure

For now, edit `components/app_config/include/AppConfig.hpp`.
Later these values should move to NVS/menuconfig:

- Wi-Fi SSID/password
- MQTT endpoint
- MQTT token/password
- station id
- command topic
- status topic
- UART pins

## Layers

```text
main
 ├─ AppConfig
 ├─ WifiManager
 ├─ MqttManager
 ├─ CabinetManager
 ├─ ProtocolManager
 └─ UartManager
```

The cabinet hardware is treated as a black box. The only required contract is the serial protocol documented in `docs/protocol_notes.md`.
