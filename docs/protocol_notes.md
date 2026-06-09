# Cabinet protocol notes

The ESP32 talks to the existing Management Unit over UART. The Management Unit and Shelf Units are treated as a black box.

## Boot sequence

```text
PFFF0e^0387.      discover Management Unit
PFFF0e_050123.    fix Management Unit addresses
P00F0v=050160.    enable V24 power to Shelf Units
P00FFe^039D.      discover Shelf Units under Management 00
P00FFe_050234.    fix Shelf Unit addresses
```

## Known-good commands

### Output pulse, 0.5 sec

| Target | Frame |
|---|---|
| Shelf 00, output 1 | `P0000o=07010516.` |
| Shelf 01, output 1 | `P0001o=07010515.` |
| Shelf 00, output 2 | `P0000o=07020515.` |
| Shelf 01, output 2 | `P0001o=07020514.` |

### LED flash

| Target | Action | Frame |
|---|---|---|
| Shelf 00 | on | `P0000f=050116.` |
| Shelf 00 | off | `P0000f=050017.` |
| Shelf 01 | on | `P0001f=050115.` |
| Shelf 01 | off | `P0001f=050016.` |

### Input read

| Target | Frame |
|---|---|
| Shelf 00 | `P0000i?0400E8.` |
| Shelf 01 | `P0001i?0400E7.` |

### Firmware read

| Target | Frame |
|---|---|
| Management Unit | `P000F#?03B8.` |
| Shelf 00 | `P0000#?03C7.` |
| Shelf 01 | `P0001#?03C6.` |

## Open issues

- Confirm UART baud rate and pins against real hardware.
- Confirm generic checksum formula before supporting arbitrary shelf/output ids.
- Confirm backend doorId format and mapping to Management/Shelf/Output.
- Confirm HTTPS endpoint for status report.
