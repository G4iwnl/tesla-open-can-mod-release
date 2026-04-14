---
sidebar_position: 3
---

# Firmware Flashing

There are two ways to flash the firmware: **Arduino IDE** (simple, GUI-based) and **PlatformIO** (for developers, supports testing and CI).

## Option A: Arduino IDE — Flash Only

Recommended if you just want to flash the firmware onto your board. No command-line tools required.

### 1. Install the Arduino IDE

Download from [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software).

### 2. Add the Board Package

**For Feather RP2040 CAN:**
1. Open **File > Preferences**.
2. In **Additional Board Manager URLs**, add:
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
3. Go to **Tools > Board > Boards Manager**, search for **Raspberry PI Pico/RP2040**, and install it.
4. Select **Adafruit Feather RP2040 CAN** as the Board.

**For Feather M4 CAN Express:**
1. In **Additional Board Manager URLs**, add:
   ```
   https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
   ```
2. Install **Adafruit SAMD Boards** from the Boards Manager.
3. Select **Feather M4 CAN (SAME51)** as the Board.
4. Install the **Adafruit CAN** library via the Library Manager.

**For ESP32 boards:**
1. In **Additional Board Manager URLs**, add:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
2. Install **esp32 by Espressif Systems** from the Boards Manager.
3. Select your ESP32 board (e.g. **ESP32 Dev Module**).

### 3. Install Required Libraries

Install via **Sketch > Include Library > Manage Libraries...**:

- **Feather RP2040 CAN:** MCP2515 by autowp
- **Feather M4 CAN Express:** Adafruit CAN
- **ESP32:** No additional libraries needed — the TWAI driver is built into the ESP32 Arduino core.

### 4. Open the Arduino Sketch

Open the `RP2040CAN` folder in Arduino IDE, or open `RP2040CAN/RP2040CAN.ino` directly. The sketch folder name matches the primary `.ino`, which keeps Arduino IDE happy.

### 5. Select Your Board and Vehicle

See [Configuration](/docs/getting-started/configuration) for details on `sketch_config.h`.

### 6. Upload

1. Connect the board via USB.
2. Select the correct board and port under **Tools**.
3. Click **Upload**.

:::tip
For Feather boards, if the board is not detected, double-press the **Reset** button to enter the UF2 bootloader, then retry the upload command. For ESP32 boards, hold the **BOOT** button during upload if auto-reset does not work.
:::

## Option B: PlatformIO — Development, Testing & Flash

For developers who want to run unit tests, build for multiple boards, or integrate with CI.

### Prerequisites (Windows)

| Tool | Purpose | Install |
|------|---------|---------|
| [Python 3](https://www.python.org/downloads/) | PlatformIO runtime | `winget install Python.Python.3.14` |
| [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) | Build system & test runner | `pip install platformio` |
| [MinGW-w64 GCC](https://winlibs.com/) | Native test compiler | `winget install BrechtSanders.WinLibs.POSIX.UCRT` |

:::tip
After installing MinGW-w64, restart your terminal so `gcc` and `g++` are on PATH. GCC is only needed for `pio test -e native` (host-side unit tests) — cross-compiling to the boards uses PlatformIO's built-in ARM toolchain.
:::

### Build

```bash
# Adafruit Feather RP2040 CAN
pio run -e feather_rp2040_can

# Adafruit Feather M4 CAN Express (ATSAME51)
pio run -e feather_m4_can

# ESP32 with TWAI (CAN) peripheral
pio run -e esp32_twai

# M5Stack Atomic CAN Base
pio run -e m5stack-atomic-can-base

# M5Stack AtomS3 CAN Base
pio run -e m5stack-atoms3-can-base
```

### Flash

Connect the board via USB, then upload:

```bash
# Adafruit Feather RP2040 CAN
pio run -e feather_rp2040_can --target upload

# Adafruit Feather M4 CAN Express
pio run -e feather_m4_can --target upload

# ESP32
pio run -e esp32_twai --target upload

# M5Stack Atomic CAN Base
pio run -e m5stack-atomic-can-base --target upload

# M5Stack AtomS3 CAN Base
pio run -e m5stack-atoms3-can-base --target upload
```

### Run Tests

```bash
pio test -e native
pio test -e native_bypass_tlssc_requirement
pio test -e native_log_buffer
```

Tests run on your host machine — no hardware required.
