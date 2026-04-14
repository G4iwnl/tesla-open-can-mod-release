---
sidebar_position: 4
---

# M5Stack Atomic CAN Base

The most compact board option. Uses the CA-IS3050G CAN transceiver over the ESP32's built-in TWAI peripheral.

## Specifications

| Property | Value |
|---|---|
| CAN Controller | ESP32 TWAI via CA-IS3050G |
| Library | ESP32 TWAI |
| Driver Define | `DRIVER_TWAI` |
| Status | Tested |

## Configuration

In `sketch_config.h`:

```cpp
#define DRIVER_TWAI
```

## PlatformIO

The M5Stack Atom and AtomS3 CAN-base builds each have their own PlatformIO environment:

```bash
pio run -e m5stack-atomic-can-base
pio run -e m5stack-atomic-can-base --target upload
pio run -e m5stack-atoms3-can-base
pio run -e m5stack-atoms3-can-base --target upload
```

## Notes

- This board uses the same `DRIVER_TWAI` define as the generic ESP32 setup
- The CAN transceiver is integrated — no external transceiver module needed
- Very small form factor, making it easy to hide in the vehicle
- The AtomS3 build uses PlatformIO board `m5stack-atoms3` with CAN-base pins `G5` (TX) and `G6` (RX)

For more details, see the [official M5Stack documentation](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base).
