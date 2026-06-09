# Backend MQTT contract

## Incoming command topic

The backend publishes door commands to:

```text
shita/stations/{stationId}/doors/command
```

Example:

```text
shita/stations/03/doors/command
```

Payload:

```json
{
  "doorId": "string",
  "activityId": "string",
  "time": "2026-06-01T08:00:00.000Z"
}
```

## Outgoing status topic

The ESP32 publishes all reports back over MQTT:

```text
shita/stations/{stationId}/doors/status
```

Example:

```text
shita/stations/03/doors/status
```

Payload examples:

```json
{
  "stationId": "03",
  "type": "READY",
  "success": true,
  "message": "cabinet_ready"
}
```

```json
{
  "stationId": "03",
  "type": "DOOR_OPEN_SENT",
  "success": true,
  "message": "door_open_command_sent",
  "doorId": "door-1",
  "activityId": "activity-123",
  "uartFrame": "P0000o=07010516.",
  "uartResponse": "p..."
}
```

## Report types

```text
BOOT
READY
COMMAND_RECEIVED
DOOR_OPEN_SENT
UART_RESPONSE
ERROR
```

## Design decision

The ESP32 does not use HTTPS in the MVP. MQTT is used both ways:

- Backend -> ESP32: commands
- ESP32 -> Backend: status, UART results, errors

This keeps the ESP32 as a realtime gateway with one persistent communication channel.
