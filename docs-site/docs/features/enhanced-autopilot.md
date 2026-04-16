---
sidebar_position: 4
---

# Enhanced Autopilot

`ENHANCED_AUTOPILOT` gates the extra autopilot-side signals this firmware handles on the CAN side. On HW3 and HW4 it controls the `UI_applyEceR79` override; on HW4 it also enables summon support.

## What It Controls

- Clears `UI_applyEceR79` on CAN message `1021`, mux `1` for HW3 and HW4
- Enables the HW4 summon bit on CAN message `1021`, mux `1`
- Keeps the normal `Traffic Light and Stop Sign Control` UI signal as the activation gate
- Adds a runtime toggle in the built-in WiFi diagnostics page when the feature is compiled in

## CAN Message Details

| CAN ID | Mux | Bit | Value | Signal |
|---|---|---|---|---|
| 1021 | 0 | 38 | read | `UI_fsdStopsControlEnabled` |
| 1021 | 1 | 19 | 0 | `UI_applyEceR79` |
| 1021 | 1 | 47 | 1 | `UI_hardCoreSummon` |

## Requirements

- **HW3 or HW4 vehicle**
- Active Tesla entitlement for the relevant Autopilot package
- `ENHANCED_AUTOPILOT` build flag enabled in `platformio.ini`

In your PlatformIO environment's `build_flags`, add:

```ini
-DENHANCED_AUTOPILOT
```

:::note
This feature does not change Tesla package entitlements. It only exposes the related CAN-side signals inside the existing firmware flow.
:::
