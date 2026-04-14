---
sidebar_position: 3
---

# Firmware Flashing

The firmware is built and flashed using **PlatformIO**.

## Prerequisites (Windows)

| Tool | Purpose | Install |
|------|---------|---------|
| [Python 3](https://www.python.org/downloads/) | PlatformIO runtime | `winget install Python.Python.3.14` |
| [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) | Build system & test runner | `pip install platformio` |
| [MinGW-w64 GCC](https://winlibs.com/) | Native test compiler | `winget install BrechtSanders.WinLibs.POSIX.UCRT` |

:::tip
After installing MinGW-w64, restart your terminal so `gcc` and `g++` are on PATH. GCC is only needed for `pio test -e native` (host-side unit tests) — cross-compiling to the boards uses PlatformIO's built-in toolchain.
:::

## Build

```bash
# Waveshare ESP32-S3 RS485/CAN (recommended)
pio run -e waveshare_esp32s3_rs485_can

# ESP32 with TWAI (CAN) peripheral
pio run -e esp32_twai

# LilyGo T-CAN485 (HW3)
pio run -e lilygo_tcan485_hw3

# M5Stack Atomic CAN Base
pio run -e m5stack-atomic-can-base

# M5Stack AtomS3 CAN Base
pio run -e m5stack-atoms3-mini-can-base
```

## Flash

Connect the board via USB, then upload:

```bash
# Waveshare ESP32-S3 RS485/CAN (recommended)
pio run -e waveshare_esp32s3_rs485_can --target upload

# ESP32
pio run -e esp32_twai --target upload

# LilyGo T-CAN485 (HW3)
pio run -e lilygo_tcan485_hw3 --target upload

# M5Stack Atomic CAN Base
pio run -e m5stack-atomic-can-base --target upload

# M5Stack AtomS3 CAN Base
pio run -e m5stack-atoms3-mini-can-base --target upload
```

:::tip
For ESP32 boards, hold the **BOOT** button during upload if auto-reset does not work.
:::

## Run Tests

```bash
pio test -e native
pio test -e native_bypass_tlssc_requirement
pio test -e native_log_buffer
```

Tests run on your host machine — no hardware required.
