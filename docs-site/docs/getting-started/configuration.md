---
sidebar_position: 4
---

# Configuration

Configuration is done via `build_flags` in `platformio.ini` for compile-time options, and through the Web UI at runtime.

## Vehicle Hardware Selection

Set your vehicle hardware variant via the `-D` build flag in your PlatformIO environment:

```ini
build_flags =
  -D HW4
```

| Define | Target | Listens on CAN IDs | Notes |
|---|---|---|---|
| `LEGACY` | HW3 Retrofit | 1006, 69 | Sets FSD enable bit and speed profile control via follow distance |
| `HW3` | HW3 vehicles | 1016, 1021 | Same functionality as legacy |
| `HW4` | HW4 vehicles | 1016, 1021 | Extended speed-profile range (5 levels) |

:::note
HW4 vehicles on firmware **2026.2.9.X** are on **FSD v14**. Versions on the **2026.8.X** branch are still on **FSD v13**. If your vehicle is running FSD v13, compile with `HW3` even if your vehicle has HW4 hardware.
:::

## Optional Features

Add optional feature flags to `build_flags`:

```ini
build_flags =
  -D HW4
  -D EMERGENCY_VEHICLE_DETECTION
  -D ENHANCED_AUTOPILOT
```

| Feature | Description |
|---|---|
| `ISA_SPEED_CHIME_SUPPRESS` | Suppresses the ISA speed chime; speed limit sign will be empty while driving |
| `EMERGENCY_VEHICLE_DETECTION` | Enables approaching emergency vehicle detection (HW4 only) |
| `BYPASS_TLSSC_REQUIREMENT` | Always enables FSD without requiring "Traffic Light and Stop Sign Control" to be toggled on |
| `ENHANCED_AUTOPILOT` | Enables the UI_applyEceR79 override on HW3/HW4, and adds summon support on HW4 |

## Serial Debug Output

Debug output is printed over Serial at **115200 baud** when `enablePrint` is `true`. Open the Serial Monitor to see live FSD state and active speed profile.
