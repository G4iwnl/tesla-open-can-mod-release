---
sidebar_position: 3
---

# Compatibility

## Vehicle Firmware Compatibility

:::warning Update
**Do not update to 2026.2.9.x & 2026.8.6 — FSD is not working on HW4.**
:::

| Firmware | HW3 | HW4 | Notes |
|---|---|---|---|
| < 2026.2.9 | FSD v12.4.6 | Compile as HW3 | HW4 vehicles use HW3 handler for FSD v13 |
| 2026.2.9.X | FSD v12.4.6 | FSD v14.x | HW4 gets full v14 features |
| 2026.8.X | FSD v12.4.6 | FSD v12.4.6 | HW4 on this branch is still v12/v13 — compile as HW3 |

## FSD Version Differences

### FSD v12/13 (HW3 handler)
- FSD enable bit
- Speed profiles (3 levels)
- Nag suppression

### FSD v14 (HW4 handler)
- FSD enable bit
- Speed profiles (5 levels)
- Nag suppression
- Enhanced Autopilot (optional)
- Actually Smart Summon (ASS)
- Emergency Vehicle Detection (optional)
- ISA Speed Chime Suppress (optional)

## Board Compatibility

All supported boards run the same shared firmware logic. The only difference is the CAN driver implementation:

| Board | PlatformIO Env | Arduino Board |
|---|---|---|
| Feather RP2040 CAN | `feather_rp2040_can` | `rp2040:rp2040:adafruit_feather_can` |
| Feather M4 CAN Express | `feather_m4_can` | Feather M4 CAN (SAME51) |
| ESP32 + Transceiver | `esp32_twai` | ESP32 Dev Module |
| M5Stack Atomic CAN Base | `m5stack-atomic-can-base` | M5Stack-ATOM |
| M5Stack AtomS3 CAN Base | `m5stack-atoms3-can-base` | M5AtomS3 |

## Third-Party Libraries

| Library | License | Used By |
|---|---|---|
| [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) | MIT | Feather RP2040 CAN |
| [adafruit/Adafruit_CAN](https://github.com/adafruit/Adafruit_CAN) | MIT | Feather M4 CAN Express |
| [espressif/esp-idf](https://github.com/espressif/esp-idf) (TWAI) | Apache 2.0 | ESP32 boards |
