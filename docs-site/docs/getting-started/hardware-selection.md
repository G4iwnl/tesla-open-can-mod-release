---
sidebar_position: 1
---

# Hardware Selection

## Supported Boards

| Board | CAN Interface | Library | Status |
|---|---|---|---|
| Adafruit Feather RP2040 CAN | MCP2515 over SPI | `mcp2515.h` (autowp) | Tested |
| Adafruit Feather M4 CAN Express (ATSAME51) | Native MCAN peripheral | `Adafruit_CAN` (`CANSAME5x`) | Tested |
| ESP32 with CAN transceiver (e.g. ESP32-DevKitC + SN65HVD230) | Native TWAI peripheral | ESP-IDF `driver/twai.h` | Tested |
| [M5Stack Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G over ESP32 TWAI | ESP32 TWAI | Tested |
| Adafruit ESP32 Feather V2 + CAN Bus Featherwing | MCP2515 over SPI | `mcp2515.h` (autowp) | Tested |
| M5Stack AtomS3 on CAN Base | CA-IS3050G over ESP32-S3 TWAI | ESP32 TWAI | Build target |

## Supported Vehicle Variants

Select your vehicle hardware variant via the `#define` directive in `sketch_config.h`.

| Define | Target | Listens on CAN IDs | Notes |
|---|---|---|---|
| `LEGACY` | HW3 Retrofit | 1006, 69 | Sets FSD enable bit and speed profile control via follow distance |
| `HW3` | HW3 vehicles | 1016, 1021 | Same functionality as legacy |
| `HW4` | HW4 vehicles | 1016, 1021 | Extended speed-profile range (5 levels) |

:::note
HW4 vehicles on firmware **2026.2.9.X** are on **FSD v14**. However, versions on the **2026.8.X** branch are still on **FSD v13**. If your vehicle is running FSD v13 (including the 2026.8.X branch or anything older than 2026.2.9), compile with `HW3` even if your vehicle has HW4 hardware.
:::

## How to Determine Your Hardware Variant

- **Legacy** — Your vehicle has a **portrait-oriented center screen** and **HW3**. This applies to older (pre Palladium) Model S and Model X vehicles that originally came with or were retrofitted with HW3.
- **HW3** — Your vehicle has a **landscape-oriented center screen** and **HW3**. You can check your hardware version under **Controls > Software > Additional Vehicle Information** on the vehicle's touchscreen.
- **HW4** — Same as above, but the Additional Vehicle Information screen shows **HW4**.

## Hardware Requirements

- One of the supported boards listed above
- CAN bus connection to the vehicle (500 kbit/s)

:::important
Cut the onboard 120 Ohm termination resistor on the Feather CAN board (jumper labeled **TERM** on RP2040, **Trm** on M4). If using an ESP32 with an external transceiver that has a termination resistor, remove or disable it as well. The vehicle's CAN bus already has its own termination, and adding a second resistor will cause communication errors.
:::

## Which Board Should I Choose?

- **Feather RP2040 CAN** — Most popular choice, well-documented, [3D printable case available](https://www.printables.com/model/1662242-adafruit-rp2040-can-bus-feather-case-5724)
- **Feather M4 CAN Express** — Native CAN controller (no SPI overhead), slightly faster
- **ESP32 + CAN transceiver** — Cheapest option if you already have an ESP32, requires external transceiver module
- **M5Stack Atomic CAN Base** — Most compact form factor
- **Feather V2 + FeatherWing** — WiFi-capable ESP32 with MCP2515 CAN controller, good balance of features and cost
- **M5Stack AtomS3 on CAN Base** — Same compact setup with the newer ESP32-S3 module
