---
sidebar_position: 5
---

# Actually Smart Summon (ASS)

On HW4 vehicles, the firmware enables Actually Smart Summon without EU regulatory restrictions.

## How It Works

The firmware sets the `UI_hardCoreSummon` bit in CAN message 1021, mux 1, which removes the EU-imposed limitation on Smart Summon functionality.

## CAN Message Details

| CAN ID | Mux | Bit | Value | Signal |
|---|---|---|---|---|
| 1021 | 1 | 47 | 1 | UI_hardCoreSummon |

## Requirements

- **HW4 vehicle only**
- Active FSD subscription or purchase
- Vehicle firmware that supports Actually Smart Summon

:::note
This feature is exposed through `ENHANCED_AUTOPILOT` on HW4 builds.
:::
