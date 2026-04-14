---
sidebar_position: 3
---

# ESP32 with CAN Transceiver

Uses the native TWAI (Two-Wire Automotive Interface) peripheral built into all ESP32 chips. Requires an external CAN transceiver module.

## Specifications

| Property | Value |
|---|---|
| CAN Controller | Native TWAI peripheral |
| Library | ESP-IDF `driver/twai.h` |
| Driver Define | `DRIVER_TWAI` |
| Transceiver | External (SN65HVD230, TJA1050, MCP2551, etc.) |
| Status | Tested |

## Required Hardware

- Any ESP32 board (e.g. ESP32-DevKitC)
- External CAN transceiver module (e.g. SN65HVD230, TJA1050, or MCP2551)

## Default Pin Configuration

| Pin | GPIO | Purpose |
|---|---|---|
| `TWAI_TX_PIN` | `GPIO_NUM_5` | Connect to transceiver TX |
| `TWAI_RX_PIN` | `GPIO_NUM_4` | Connect to transceiver RX |

If you have other GPIO Pins connected to the CAN transceiver Module. You can define the Ports in `/include/arduino_enytrypoint.h`

## Configuration

In `sketch_config.h`:

```cpp
#define DRIVER_TWAI
```

## Arduino IDE Setup

1. Add board URL: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. Install **esp32 by Espressif Systems** from Boards Manager
3. Select your ESP32 board (e.g. **ESP32 Dev Module**)
4. No additional libraries needed — the TWAI driver is built into the ESP32 Arduino core

## PlatformIO

```bash
pio run -e esp32_twai
pio run -e esp32_twai --target upload
```

:::tip
Hold the **BOOT** button during upload if auto-reset does not work.
:::

:::important
If your CAN transceiver module has an onboard termination resistor, remove or disable it. The vehicle's CAN bus already has its own termination.
:::
