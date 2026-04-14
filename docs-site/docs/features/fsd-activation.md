---
sidebar_position: 1
---

# FSD Activation

The core feature of Tesla Open CAN Mod — enabling Full Self-Driving at the CAN bus level.

## How It Works

1. The firmware listens for Autopilot-related CAN frames on the vehicle bus.
2. It checks if **"Traffic Light and Stop Sign Control"** is enabled in the vehicle's Autopilot settings.
3. When enabled, it sets the FSD enable bit in the appropriate CAN message.
4. The modified frame is re-transmitted onto the vehicle bus.

## CAN Message Details

### Legacy (HW3 Retrofit)

| CAN ID | Mux | Bit | Value | Description |
|---|---|---|---|---|
| 1006 | 0 | 38 | (0/1) | Read FSD state |
| 1006 | 0 | 46 | 1 | Enable FSD |

### HW3

| CAN ID | Mux | Bit | Value | Signal | Description |
|---|---|---|---|---|---|
| 1021 | 0 | 38 | (0/1) | UI_fsdStopsControlEnabled | Read FSD state |
| 1021 | 0 | 46 | 1 | | Enable FSD |

### HW4

| CAN ID | Mux | Bit | Value | Description |
|---|---|---|---|---|
| 1021 | 0 | 38 | (0/1) | Read FSD state |
| 1021 | 0 | 46 | 1 | Enable FSD |
| 1021 | 0 | 59 | 1 | Enable detection |
| 1021 | 0 | 60 | 1 | Enable V14 |

:::important
If a CAN message contains a counter or checksum, any modification to that message **must** recalculate these fields — otherwise the receiving ECU will discard the frame.
:::

## BYPASS_TLSSC_REQUIREMENT Mode

By default, FSD is only activated when "Traffic Light and Stop Sign Control" is toggled on in the vehicle settings. If you want FSD to be always active, enable `BYPASS_TLSSC_REQUIREMENT` in `sketch_config.h`:

```cpp
#define BYPASS_TLSSC_REQUIREMENT
```

This bypasses the toggle check and always sets the FSD enable bit.
