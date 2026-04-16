---
sidebar_position: 7
---

# ISA Speed Chime Suppression

Suppresses the ISA (Intelligent Speed Assistance) speed warning chime on HW4 vehicles.

## How It Works

The ISA system monitors speed limits and alerts the driver when exceeding them. The firmware can suppress the audible chime while still showing the visual indicator on the dashboard.

The suppression is controlled via CAN message **921 (DAS_status)**, which contains the `DAS_suppressSpeedWarning` signal.

## CAN Message Details

| CAN ID | Bit | Value | Signal | Description |
|---|---|---|---|---|
| 921 | 13 | 1 | DAS_suppressSpeedWarning | Suppress speed warning chime |
| 921 | 56-63 | (checksum) | DAS_statusChecksum | Update checksum |

:::important
CAN message 921 includes a checksum in bits 56-63. When modifying this message, the checksum **must be recalculated** — otherwise the receiving ECU will discard the frame.
:::

## Configuration

This feature is optional and must be explicitly enabled at compile time via PlatformIO build flag:

```ini
-DISA_SPEED_CHIME_SUPPRESS
```

Then build and flash:

```bash
pio run -e esp32_twai --target upload
```

## Runtime Control (Web Interface)

On ESP32 boards with the web interface enabled, you can toggle ISA chime suppression at runtime:

1. Open the web interface: `http://192.168.4.1` (WiFi: `TeslaCAN` / `canmod12`)
2. Navigate to **Controls** section
3. Toggle **ISA Speed Chime Suppress** on/off

The setting is persistent and will be restored after reboot.

## Requirements

- **HW4 vehicle only** — this feature is not available on HW3 or Legacy
- **ESP32 board** — OTA control via web interface requires ESP32 (RP2040/M4 can only enable at compile time)
- Vehicle firmware that supports ISA feature

## Behavior

When enabled:
- The **audible chime** is suppressed when exceeding speed limits
- The **visual indicator** (speed limit sign on dashboard) remains active
- The driver is still visually informed of speed limit violations

:::caution
Always stay attentive to speed limits. This feature suppresses sound only — the visual indicator is still present. Do not rely solely on the chime for speed awareness while driving.
:::
