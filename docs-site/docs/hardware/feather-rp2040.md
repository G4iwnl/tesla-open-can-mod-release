---
sidebar_position: 1
---

# Adafruit Feather RP2040 CAN

The most popular board choice for this project. Uses the MCP2515 CAN controller over SPI.

## Specifications

| Property | Value |
|---|---|
| CAN Controller | MCP2515 over SPI |
| Library | `mcp2515.h` (autowp) |
| Driver Define | `DRIVER_MCP2515` |
| Status | Tested |
| Case | [3D Printable (Printables)](https://www.printables.com/model/1662242-adafruit-rp2040-can-bus-feather-case-5724) |

## Required Pins

The board must expose these pins (defined by the earlephilhower board variant):

| Pin | Purpose |
|---|---|
| `PIN_CAN_CS` | SPI chip-select for the MCP2515 |
| `PIN_CAN_INTERRUPT` | Interrupt pin (unused; polling mode) |
| `PIN_CAN_STANDBY` | CAN transceiver standby control |
| `PIN_CAN_RESET` | MCP2515 hardware reset |

## Configuration

In `sketch_config.h`:

```cpp
#define DRIVER_MCP2515
```

## Arduino IDE Setup

1. Add board URL: `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
2. Install **Raspberry PI Pico/RP2040** from Boards Manager
3. Select **Adafruit Feather RP2040 CAN** as board
4. Install **MCP2515 by autowp** library

## PlatformIO

```bash
pio run -e feather_rp2040_can
pio run -e feather_rp2040_can --target upload
```

:::important
Cut the onboard 120 Ohm termination resistor (jumper labeled **TERM**). The vehicle's CAN bus already has its own termination.
:::
