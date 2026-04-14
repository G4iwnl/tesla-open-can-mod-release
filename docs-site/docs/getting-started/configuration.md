---
sidebar_position: 4
---

# Configuration

All configuration is done in a single file: `RP2040CAN/sketch_config.h`. This file is shared between Arduino IDE and PlatformIO.

## Board Selection

Uncomment **one** of the following lines to match your board:

```cpp
#define DRIVER_MCP2515   // Adafruit Feather RP2040 CAN (MCP2515 over SPI)
//#define DRIVER_SAME51  // Adafruit Feather M4 CAN Express (native ATSAME51 CAN)
//#define DRIVER_TWAI    // ESP32 boards with built-in TWAI (CAN) peripheral
```

## Vehicle Hardware Selection

Uncomment **one** of the following lines to match your vehicle:

```cpp
#define LEGACY // HW3-retrofit (portrait screen)
//#define HW3  // HW3 (landscape screen)
//#define HW4  // HW4 (landscape screen)
```

:::note
HW4 vehicles on firmware **2026.2.9.X** are on **FSD v14**. Versions on the **2026.8.X** branch are still on **FSD v13**. If your vehicle is running FSD v13, compile with `HW3` even if your vehicle has HW4 hardware.
:::

## Optional Features

Uncomment any of the following lines to enable optional features:

```cpp
// #define ISA_SPEED_CHIME_SUPPRESS         // Suppress ISA speed chime
// #define EMERGENCY_VEHICLE_DETECTION      // Enable emergency vehicle detection (HW4)
// #define BYPASS_TLSSC_REQUIREMENT          // Always enable FSD without requiring toggle
// #define ENHANCED_AUTOPILOT               // Enable UI_applyEceR79 override on HW3/HW4 and summon support on HW4
```

| Feature | Description |
|---|---|
| `ISA_SPEED_CHIME_SUPPRESS` | Suppresses the ISA speed chime; speed limit sign will be empty while driving |
| `EMERGENCY_VEHICLE_DETECTION` | Enables approaching emergency vehicle detection (HW4 only) |
| `BYPASS_TLSSC_REQUIREMENT` | Always enables FSD without requiring "Traffic Light and Stop Sign Control" to be toggled on |
| `ENHANCED_AUTOPILOT` | Enables the UI_applyEceR79 override on HW3/HW4, and adds summon support on HW4 |

## PlatformIO Command-Line Configuration

You can switch the configuration from the command line instead of editing the file by hand:

```bash
python3 scripts/platformio_set_ino_profile.py \
  --driver DRIVER_MCP2515 \
  --vehicle HW4 \
  --enable EMERGENCY_VEHICLE_DETECTION \
  --enable ENHANCED_AUTOPILOT
```

PlatformIO reads the active board, vehicle, and optional feature defines from `sketch_config.h`. The `-e` environment still selects the board, so it must match the uncommented driver define. If they do not match, the build stops with a clear error.

## Serial Debug Output

Debug output is printed over Serial at **115200 baud** when `enablePrint` is `true`. Open the Serial Monitor to see live FSD state and active speed profile.
