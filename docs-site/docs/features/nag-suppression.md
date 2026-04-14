---
sidebar_position: 2
---

# Nag Suppression

The firmware clears the hands-on-wheel nag bit, suppressing the periodic "Apply pressure to the steering wheel" warning.

## How It Works

The nag is controlled by the `UI_applyEceR79` signal in CAN message 1021 (or 1006 on Legacy). The firmware sets this bit to `0` on every relevant frame, preventing the nag from triggering.

On HW3 and HW4, this override is part of the `ENHANCED_AUTOPILOT` option. On Legacy it remains always active.

## CAN Message Details

| Variant | CAN ID | Mux | Bit | Value | Signal |
|---|---|---|---|---|---|
| Legacy | 1006 | 1 | 19 | 0 | Suppress nag |
| HW3 | 1021 | 1 | 19 | 0 | UI_applyEceR79 (`ENHANCED_AUTOPILOT`) |
| HW4 | 1021 | 1 | 19 | 0 | UI_applyEceR79 (`ENHANCED_AUTOPILOT`) |

:::caution
Always keep your hands on the wheel and stay attentive while driving. This feature is for testing purposes only. You are responsible for safe vehicle operation at all times.
:::
