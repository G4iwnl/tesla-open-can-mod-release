---
sidebar_position: 5
---

# Adafruit ESP32 Feather V2 + CAN Bus Featherwing

Uses the MCP2515 CAN controller over SPI via the Adafruit CAN Bus Featherwing stacked on top of the ESP32 Feather V2.

## Specifications

| Property | Value |
|---|---|
| Main Board | [Adafruit ESP32 Feather V2 (5400)](https://www.adafruit.com/product/5400) |
| CAN Add-on | [Adafruit CAN Bus Featherwing (5709)](https://www.adafruit.com/product/5709) |
| CAN Controller | MCP2515 over SPI |
| Library | `mcp2515.h` (autowp) |
| Driver Define | `DRIVER_MCP2515` |
| Status | Tested |

## Required Hardware

- Adafruit ESP32 Feather V2
- Adafruit CAN Bus Featherwing
- Stacking headers (to connect the Featherwing to the Feather V2)

## Pin Configuration

| Pin | GPIO | Purpose |
|---|---|---|
| `PIN_CAN_CS` | `14` | SPI chip-select for the MCP2515 |
| `PIN_CAN_INTERRUPT` | `32` | Interrupt pin (unused; polling mode) |
| `PIN_LED` | `13` | Onboard LED |

Ensure the stacking headers are installed and the SPI bus is properly connected between the Feather V2 and the Featherwing.

## Configuration

In `sketch_config.h`:

```cpp
#define DRIVER_MCP2515
```

## Arduino IDE Setup

1. Add board URL: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. Install **esp32 by Espressif Systems** from Boards Manager
3. Select **Adafruit ESP32 Feather V2** as board
4. Install **MCP2515 by autowp** library

## PlatformIO

```bash
pio run -e esp32_feather_v2_mcp2515
pio run -e esp32_feather_v2_mcp2515 --target upload
```

:::tip
Hold the **BOOT** button during upload if auto-reset does not work.
:::

:::important
If the CAN Bus Featherwing has an onboard termination resistor, cut or disable it. The vehicle's CAN bus already has its own termination, and adding a second resistor will cause communication errors.
:::
