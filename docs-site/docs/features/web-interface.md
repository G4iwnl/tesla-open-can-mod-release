---
sidebar_position: 6
---

# Web Interface (ESP32)

ESP32 boards (and M5Stack Atomic CAN Base) feature an optional web interface for remote monitoring and control via WiFi.

![esp32web server](/img/webserver.png)

## Accessing the Web Interface

1. **Power on the board** — it will create a WiFi hotspot
2. **Connect to WiFi**:
   - Network: `TeslaCAN`
   - Password: `canmod12`
3. **Open a browser** and navigate to: `http://192.168.4.1`

## Features

### Real-Time Status

- **FSD Active**: Current FSD state
- **Bypass TLSSC**: Whether the TLSSC requirement is bypassed
- **ISA Speed Chime Suppress**: Suppression status (HW4 only)
- **Emergency Vehicle Detection**: Detection status (HW4 only)
- **Speed Profile**: Current active speed level
- **Speed Offset**: Current offset value
- **Uptime**: Board uptime in hours:minutes:seconds

### Remote Controls

#### Bypass TLSSC Toggle

Enable or disable FSD activation without the "Traffic Light and Stop Sign Control" requirement.

- **Build-enabled**: Always available
- **Runtime enabled**: Can be toggled on/off via web UI

#### ISA Speed Chime Suppress (HW4 only)

Suppress the ISA (Intelligent Speed Assistance) speed warning chime.

- **Build support**: Requires `ISA_SPEED_CHIME_SUPPRESS` define at compile time
- **Runtime control**: Toggle on/off via web UI if supported

#### Emergency Vehicle Detection (HW4 only)

Enable emergency vehicle detection and response on HW4 vehicles with FSD v14.

- **Build support**: Requires `EMERGENCY_VEHICLE_DETECTION` define at compile time
- **Runtime control**: Toggle on/off via web UI if supported

#### Log Toggle

Enable or disable serial and web log output.

### Live Log

Real-time scrolling log showing debug output from the firmware. Displays timestamps and log messages.

## OTA (Over-The-Air) Firmware Updates

Update your board's firmware wirelessly without needing a USB cable.

### How to Update

1. **Compile your firmware** locally using PlatformIO:

   ```bash
   pio run -e esp32_twai
   ```

   This creates a binary file in `.pio/build/esp32_twai/firmware.bin`

2. **Open the web interface** and navigate to the "OTA Update" section

3. **Select the firmware file** and click "Upload Firmware"

4. **Wait for completion** — the board will reboot automatically after a successful update

### Important Notes

- The board **requires WiFi connectivity** during the update
- Do **not power off or disconnect WiFi** during the update process
- A successful update message will appear in the OTA Status area
- If the update fails, the board will retain the previous firmware
- In the sketch_config.sh you can enable all the things, by default on the esp32 it will be turned off or on, selected in the webUI.

## Available Endpoints

The web interface exposes the following API endpoints:

- `GET /api/status` — Get current state (FSD, profile, uptime, etc.)
- `POST /api/bypass-tlssc` — Toggle Bypass TLSSC
- `POST /api/isa-speed-chime-suppress` — Toggle ISA chime (HW4 only)
- `POST /api/emergency-vehicle-detection` — Toggle emergency detection (HW4 only)
- `GET /api/log` — Retrieve recent log lines

## Building with Web Interface

The web interface is only compiled when using **DRIVER_TWAI** (ESP32 boards).

To enable it, use the PlatformIO environment:

```bash
pio run -e esp32_twai
pio run -e m5stack-atomic-can-base
```

Other boards (RP2040, M4) do not have web interface support.
