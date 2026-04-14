---
sidebar_position: 5
---

# Emergency Vehicle Detection

On HW4 vehicles with FSD v14, the firmware can enable Approaching Emergency Vehicle Detection.

## How It Works

The firmware sets the detection enable bit in CAN message 1021, mux 0, which activates the vehicle's ability to detect and respond to approaching emergency vehicles.

## CAN Message Details

| CAN ID | Mux | Bit | Value | Description |
|---|---|---|---|---|
| 1021 | 0 | 59 | 1 | Enable detection |

## Configuration

This feature is optional and must be explicitly enabled in `sketch_config.h`:

```cpp
#define EMERGENCY_VEHICLE_DETECTION
```

## Requirements

- **HW4 vehicle only**
- **FSD v14** (firmware 2026.2.9.X or later)
- `EMERGENCY_VEHICLE_DETECTION` define enabled in configuration
