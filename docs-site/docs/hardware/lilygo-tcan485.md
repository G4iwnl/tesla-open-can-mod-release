---
sidebar_position: 6
---

# LILYGO T-CAN485

The LILYGO T-CAN485 is a compact ESP32-based board with an integrated CAN bus transceiver and RS485 interface. It uses the ESP32's built-in TWAI (Two-Wire Automotive Interface) controller.

## Specifications

| Property | Value |
|---|---|
| Main Board | [LILYGO T-CAN485](https://www.lilygo.cc/products/t-can485) |
| CAN Controller | ESP32 TWAI (built-in) |
| CAN TX Pin | GPIO 27 |
| CAN RX Pin | GPIO 26 |
| LED Pin | GPIO 2 |
| Driver Define | `DRIVER_TWAI` |
| Status | Supported |

## Build Environments

Two pre-configured environments are available in `platformio.ini`:

| Environment | Use Case |
|---|---|
| `lilygo_tcan485_nag_killer` | Autosteer Nag Killer (CAN bus 4, X179 pin 2/3) |
| `lilygo_tcan485_hw3` | FSD bypass for HW3 vehicles (CAN bus 6, X179 pin 13/14) |

## Flashing

```bash
# Nag Killer build
pio run -e lilygo_tcan485_nag_killer -t upload

# HW3 FSD bypass build
pio run -e lilygo_tcan485_hw3 -t upload
```

## Notes

- The T-CAN485 board has an onboard CAN transceiver — no additional hardware is needed.
- Connect directly to the vehicle's CAN bus via the screw terminal or JST connector.
- The RS485 interface on the board is not used by this firmware.
