---
sidebar_position: 2
---

# Adafruit Feather M4 CAN Express

Uses the native ATSAME51 CAN (MCAN) peripheral with an onboard TJA1051T/3 transceiver. No SPI overhead — direct CAN bus access.

## Specifications

| Property | Value |
|---|---|
| CAN Controller | Native MCAN peripheral (ATSAME51) |
| Library | `Adafruit_CAN` (`CANSAME5x`) |
| Driver Define | `DRIVER_SAME51` |
| Transceiver | TJA1051T/3 (onboard) |
| Status | Tested |

## Required Pins

| Pin | Purpose |
|---|---|
| `PIN_CAN_STANDBY` | CAN transceiver standby control |
| `PIN_CAN_BOOSTEN` | 3V to 5V boost converter enable for CAN signal levels |

## Configuration

In `sketch_config.h`:

```cpp
#define DRIVER_SAME51
```

## Arduino IDE Setup

1. Add board URL: `https://adafruit.github.io/arduino-board-index/package_adafruit_index.json`
2. Install **Adafruit SAMD Boards** from Boards Manager
3. Select **Feather M4 CAN (SAME51)** as board
4. Install **Adafruit CAN** library

## PlatformIO

```bash
pio run -e feather_m4_can
pio run -e feather_m4_can --target upload
```

:::important
Cut the onboard 120 Ohm termination resistor (jumper labeled **Trm**). The vehicle's CAN bus already has its own termination.
:::
