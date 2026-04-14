---
sidebar_position: 2
---

# Troubleshooting

## Board Not Detected

**Feather boards:** Double-press the **Reset** button to enter the UF2 bootloader, then retry the upload command.

**ESP32 boards:** Hold the **BOOT** button during upload if auto-reset does not work.

## No Serial Output

- Ensure the Serial Monitor is set to **115200 baud**
- Check that `enablePrint` is set to `true`
- Verify the USB cable supports data (not charge-only)

## CAN Communication Errors

- **Check termination:** Make sure you cut/removed the onboard 120 Ohm termination resistor
- **Check wiring:** CAN-H and CAN-L must not be swapped
- **Check baud rate:** The vehicle uses 500 kbit/s — this is set automatically by the firmware

## FSD Not Activating

1. Verify you have an active FSD subscription or purchase
2. Check that "Traffic Light and Stop Sign Control" is enabled in Autopilot settings (unless using `BYPASS_TLSSC_REQUIREMENT`)
3. Open the Serial Monitor and check if the handler output shows `FSD: 1`
4. Verify you selected the correct vehicle variant in `sketch_config.h`
5. HW4 on FSD v13: Make sure you compiled with `HW3`, not `HW4`

## Build Errors

### Board/driver mismatch

If the PlatformIO `-e` environment doesn't match the `DRIVER_*` define in `sketch_config.h`, the build will fail with a clear error. Make sure they match.

### Missing libraries

- **Feather RP2040:** Install **MCP2515 by autowp** in Arduino IDE
- **Feather M4:** Install **Adafruit CAN** in Arduino IDE
- **ESP32:** No additional libraries needed

### Native test compilation fails (Windows)

Install MinGW-w64 GCC:
```bash
winget install BrechtSanders.WinLibs.POSIX.UCRT
```
Restart your terminal so `gcc` is on PATH.

## Speed Profile Not Changing

- Make sure you're adjusting the **follow distance** on the steering wheel stalk, not cruise speed
- Check Serial Monitor output to see if the distance value changes
- Verify the correct vehicle variant is selected (HW4 has 5 levels, HW3 has 3)
