---
sidebar_position: 1
---

# Hardware Selection

## Supported Boards

| Board | CAN Interface | Status |
|---|---|---|
| [Waveshare ESP32-S3 RS485/CAN](https://www.waveshare.com/esp32-s3-relay-6ch.htm) | Built-in SIT1051T transceiver via ESP32-S3 TWAI | Recommended |
| ESP32 with CAN transceiver (e.g. ESP32-DevKitC + SN65HVD230) | Native TWAI peripheral | Tested |
| [M5Stack Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G over ESP32 TWAI | Tested |
| M5Stack AtomS3 on CAN Base | CA-IS3050G over ESP32-S3 TWAI | Tested |

## Supported Vehicle Variants

Select your vehicle hardware variant via the `build_flags` in `platformio.ini` or the Web UI at runtime.

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
If your ESP32 board or external CAN transceiver has a 120 Ohm termination resistor, remove or disable it. The vehicle's CAN bus already has its own termination, and adding a second resistor will cause communication errors.
:::

## Which Board Should I Choose?

- **Waveshare ESP32-S3 RS485/CAN** — Recommended. Built-in CAN transceiver, WiFi, Web UI, 16MB flash, compact form factor
- **M5Stack Atomic CAN Base** — Most compact form factor
- **M5Stack AtomS3 on CAN Base** — Compact setup with ESP32-S3
- **ESP32 + CAN transceiver** — Cheapest option if you already have an ESP32, requires external transceiver module
