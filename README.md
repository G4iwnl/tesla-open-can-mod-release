### 🚨 DO NOT UPDATE YOUR TESLA TO `2026.8.6`, `2026.2.9.x` OR `2026.2.300` TO KEEP FSD FEATURES 🚨

<br>
<hr>

# Tesla Open Can Mod

> This project is a fork of [Tesla-OPEN-CAN-MOD](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod). Thanks to the original author for the open-source contribution.

An open-source general-purpose CAN bus modification tool for Tesla vehicles. While FSD enablement was the starting point, the goal is to expose and control everything accessible via CAN — speed control, nag suppression, battery preconditioning, ISA chime mute, emergency vehicle awareness, and more.

**Current Version:** 1.6.1 · **Platform:** ESP32 / ESP32-S3 · **Interface:** TWAI (CAN) · **Language:** C++ (PlatformIO + Arduino framework)

## Disclaimer

> [!WARNING]
> **FSD is a premium feature and must be properly purchased or subscribed to.** Any attempt to bypass the purchase or subscription requirement will result in a permanent ban from Tesla services.

> [!WARNING]
> **Modifying CAN bus messages can cause dangerous behavior or permanently damage your vehicle.** The CAN bus controls everything from braking and steering to airbags — a malformed message can have serious consequences. If you don't fully understand what you're doing, **do not install this on your car**.

This project is for testing and educational purposes only and for use on **private property**. The authors accept no responsibility for any damage to your vehicle, injury, or legal consequences resulting from the use of this software. This project may void your vehicle warranty and **may not comply with road safety regulations in your jurisdiction**.

For any use beyond private testing, you are responsible for complying with all applicable local laws and regulations. Always keep your hands on the wheel and stay attentive while driving.

## Features

### CAN Bus Modifications

- **FSD Activation** — Enables Full Self-Driving at the CAN bus level by intercepting and modifying Autopilot control frames (0x3FD / 0x3EE)
- **Nag Suppression** — Clears the hands-on-wheel ECE R79 nag bit, suppressing the periodic "apply pressure to the steering wheel" warning
- **Autosteer Nag Killer** — Suppresses the Autopilot "hands on wheel" alert by echoing a modified EPAS3P_sysStatus frame (0x370) with spoofed torque (1.80 Nm) and incremented counter on CAN bus 4
- **Enhanced Autopilot** — Enables Enhanced Autopilot features including Summon (HW3/HW4, optional)
- **Speed Profiles** — 5 aggressiveness profiles (Chill / Normal / Hurry / Max / Sloth); auto mode maps follow-distance stalk setting, or manual mode via web UI
- **Speed Offset (Manual)** — Adjustable 0–50% speed offset applied on top of the Autopilot set speed (HW3/HW4)
- **Smart Speed Offset** — Up to 8 speed-based rules that automatically adjust offset based on the current fused speed limit. Default rules: ≤40→50%, 50→30%, 60→20%, 80+→10%
- **ISA Speed Chime Suppression** — Mutes the Intelligent Speed Assistance audible chime while keeping the visual indicator active (HW4, optional)
- **Emergency Vehicle Detection** — Enables approaching emergency vehicle detection on FSD v14 (HW4, optional)
- **Battery Preheat** — Triggers BMS battery preconditioning by injecting UI_tripPlanning frames (0x082) every 500 ms. Configurable auto-stop by temperature (-5–25°C) and max duration (10–60 min)
- **China Mode** — Bypasses traffic light/stop sign UI selection check for China-region firmware

### Web Interface

A built-in WiFi hotspot (AP mode, SSID `TeslaCAN`) serves a full-featured single-page web dashboard. Optional STA mode allows connecting to a home/garage WiFi network.

**Dashboard Tab**
- Real-time signal monitoring: throttle, speed limits, hands-on level, follow distance, gear
- Battery status: SoC%, voltage, current, temperature min/max
- Live speed/SoC/power chart (Canvas, 120-point ring buffer)
- CAN frame viewer: filterable live table with hex data, frame count, and frequency (Hz)
- Speed profile selector and offset controls

**Settings Tab**
- Runtime hardware mode switch (Legacy / HW3 / HW4) with NVS persistence
- Feature toggles: Bypass TLSSC, ISA Chime, Emergency Vehicle, Enhanced AP, Nag Killer, China Mode
- Preheat controls with status panel (elapsed time, battery temp, auto-stop config)

**System Tab**
- System info: chip temperature, handler latency (avg/max), free heap/PSRAM
- WiFi SSID/password configuration
- OTA firmware upload with progress bar and MD5 verification
- Drive data recording: ring buffer with CSV export (speed, accel, steer, battery, temps)
- Live log viewer (256-entry ring buffer)

**UI Features**
- Auto dark/light theme following vehicle headlight state (CAN 0x185)
- Bilingual i18n: Simplified Chinese / English, switchable at runtime
- Captive portal support (iOS/Android auto-redirect)

### Safety & Reliability

- **OTA Protection** — Automatically pauses all CAN modifications when a vehicle OTA update is detected (GTW_carState 0x318); auto-resumes after completion
- **NVS Corruption Recovery** — Detects NVS flash corruption on boot, auto-erases and reinitializes with defaults
- **Preheat Safeguards** — Auto-stop by temperature threshold and max duration; not persisted on reboot (requires manual re-enable)
- **Speed Offset Warning** — Safety modal on first non-zero offset use
- **Rate Limiting** — 500 ms minimum interval on all POST endpoints (429 on overflow)
- **Single-Shot TX** — Preheat injection uses TWAI_MSG_FLAG_SS to prevent CAN retry storms

### Performance

- **O(1) CAN Filtering** — 32-element bitmask array covers IDs 0–1023; no linear search per frame
- **O(1) Signal Lookup** — 1024-element direct-mapped table for CAN live signal updates
- **Zero-Allocation JSON** — canLiveHandler and statusHandler use snprintf with static buffers (no malloc per request)
- **Single Decode Pass** — OTA check, smart offset, and preheat share one `decodeSignals()` call per loop iteration
- **Canvas Throttling** — Chart and CAN table use `requestAnimationFrame` with visibility check; skip redraw when tab is hidden
- **Cached DOM Queries** — Repeated `querySelectorAll` results are cached on first access

## CAN Signal Reference

All CAN signals used by this firmware are listed below. **Modified** signals are intercepted and re-transmitted with altered bits. **Injected** signals are generated by the firmware. **Monitor-only** signals are passively decoded for the web dashboard.

### Modified & Injected Signals

| CAN ID | Name | Direction | Handler | Function |
|--------|------|-----------|---------|----------|
| `0x045` (69) | STW_ACTN_RQ | Read | Legacy | Reads follow-distance stalk position to map speed profiles |
| `0x082` (130) | UI_tripPlanning | **Inject** | Preheat | Injected every 500 ms with `byte[0]=0x05` to trigger BMS battery preconditioning |
| `0x313` (787) | TrackModeRequest | **Modify** | HW3 | Sets track-mode request bit (`byte[0] bit 0-1 = 0x01`) and recalculates checksum |
| `0x370` (880) | EPAS3P_sysStatus | **Inject** | NagKiller | When `handsOnLevel=0`, echoes a spoofed frame with `torsionBarTorque=1.80 Nm`, `handsOnLevel=1`, counter+1, and recalculated checksum |
| `0x399` (921) | DAS_status | **Modify** | HW4 | ISA speed chime suppression: sets `byte[1] \| 0x20` and recalculates checksum |
| `0x3EE` (1006) | UI_autopilotControl | **Modify** | Legacy | Mux 0: enables FSD (`bit 46`), sets speed profile. Mux 1: clears ECE R79 nag (`bit 19`) |
| `0x3F8` (1016) | UI_driverAssistControl | Read | HW3 / HW4 | Reads follow-distance setting (`byte[5] bits 5-7`) for auto speed-profile mapping |
| `0x3FD` (1021) | UI_autopilotControl | **Modify** | HW3 / HW4 | Mux 0: enables FSD (`bit 46`), reads/writes speed offset, emergency vehicle detection (`bit 59`, HW4). Mux 1: clears ECE R79 nag (`bit 19`), enables enhanced AP (`bit 47`, HW4). Mux 2: writes speed offset and speed profile |

### Monitor-Only Signals (Web Dashboard)

These signals are passively decoded from the CAN bus for the real-time dashboard. They are **never modified or re-transmitted**.

| CAN ID | Name | Decoded Fields | Unit |
|--------|------|----------------|------|
| `0x118` (280) | DI_systemStatus | Gear position, accelerator pedal position | —, % |
| `0x132` (306) | BMS_hvBusStatus | Pack voltage, pack current | V, A |
| `0x185` (389) | Lighting | Light state, raw bytes 3-4 | — |
| `0x238` (568) | UI_driverAssistMapData | Map speed limit | kph |
| `0x257` (599) | DI_speed | Vehicle speed, UI display speed, speed units | kph, kph/mph |
| `0x292` (658) | BMS_socStatus | Battery state of charge | % |
| `0x2B9` (697) | DAS_control | DAS set speed, ACC state | kph, enum |
| `0x312` (786) | BMS_thermalStatus | Battery temp min / max | °C |
| `0x318` (792) | GTW_carState | OTA update in progress flag | bool |
| `0x334` (820) | UI_powertrainControl | Vehicle speed limit | kph |
| `0x370` (880) | EPAS3P_sysStatus | Hands-on level, torsion bar torque, counter | enum, Nm, — |
| `0x389` (905) | DAS_status2 | ACC speed limit | kph |
| `0x399` (921) | DAS_status | Fused speed limit, vision speed limit, suppress speed warning | kph, kph, bool |
| `0x3D9` (985) | UI_gpsVehicleSpeed | GPS speed, user speed offset, MPP speed limit | kph, kph, kph |
| `0x3F8` (1016) | UI_driverAssistControl | Follow distance setting | 1-7 |
| `0x3FD` (1021) | UI_autopilotControl | FSD UI selection, FSD enabled, HW4 lock, speed profile, speed offset, ECE R79 nag | — |

> **Note:** Signals `0x145` (ESP_status brake torque) and `0x261` (motor torque) are defined in the decoder but currently disabled because their byte positions have not been verified on the X179 CAN bus.

## How It Works

The firmware runs on ESP32 and ESP32-S3 boards with the native TWAI (CAN) peripheral. It sits on the vehicle CAN bus, intercepts relevant frames, modifies the necessary bits, and re-transmits the modified frames — all in real time.

A bitmask-based O(1) filter determines which CAN IDs the active handler needs. Non-matching frames pass through untouched. The hardware variant (Legacy / HW3 / HW4) can be set at build time or switched at runtime (ESP32-S3 boards with `RUNTIME_HW_SWITCH`). A WiFi web interface provides runtime control, real-time monitoring, and over-the-air firmware updates.

### Architecture

```
Vehicle CAN Bus
      │
  ┌───┴───┐
  │ TWAI  │  ESP32/ESP32-S3
  │ Driver │
  └───┬───┘
      │
  ┌───┴──────────┐
  │  App Loop    │  ~1 ms cycle
  │  ┌────────┐  │
  │  │ Filter │──┤── O(1) bitmask check
  │  └───┬────┘  │
  │  ┌───┴────┐  │
  │  │Handler │──┤── Legacy / HW3 / HW4
  │  └───┬────┘  │
  │  ┌───┴────┐  │
  │  │Decode  │──┤── Signal monitor (1 pass/loop)
  │  └────────┘  │
  └──────────────┘
      │
  ┌───┴──────────┐
  │  Web Server  │  HTTP on :80
  │  29 endpoints│
  │  WiFi AP+STA │
  └──────────────┘
```

## Prerequisites

**An active FSD package is required to use FSD-related features** — either purchased or subscribed. This board enables the FSD functionality on the CAN bus level, but the vehicle still needs a valid FSD entitlement from Tesla.

Features like the Autosteer Nag Killer, ISA Speed Chime Suppression, Battery Preheat, and the Web Interface work independently and do not require FSD.

If FSD subscriptions are not available in your region, there is a workaround using a foreign Tesla account.

## Supported Boards

| Board                                                                   | CAN Interface              | PSRAM | Status      |
|-------------------------------------------------------------------------|----------------------------|-------|-------------|
| [Waveshare ESP32-S3 RS485/CAN](https://www.waveshare.com/esp32-s3-rs485-can.htm) | SIT1050T + ESP32-S3 TWAI   | 8 MB  | Recommended |
| ESP32 + CAN transceiver (e.g. ESP32-DevKitC + SN65HVD230)               | Native TWAI peripheral     | —     | Tested      |
| [Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G + ESP32 TWAI    | —     | Tested      |
| M5Stack AtomS3 + CAN Base                                               | CA-IS3050G + ESP32-S3 TWAI | —     | Tested      |
| LilyGo T-CAN485                                                         | ESP32 TWAI + SN65HVD230    | —     | Tested      |

## Build & Flash

Use PlatformIO to build and flash. Select your board environment and configure the vehicle variant in `platformio.ini` build flags.

```bash
# Build for Waveshare ESP32-S3
pio run -e waveshare_esp32s3_rs485_can

# Build for generic ESP32
pio run -e esp32_twai

# Flash over USB
pio run -e waveshare_esp32s3_rs485_can -t upload

# Run unit tests (114 native tests)
pio test -e native
```

### Build Flags

| Flag | Default | Description |
|------|---------|-------------|
| `-DHW4` | On (esp32_twai) | Target HW4 vehicle handler |
| `-DBYPASS_TLSSC_REQUIREMENT` | Off | Bypass Traffic Light & Stop Sign Control UI toggle |
| `-DISA_SPEED_CHIME_SUPPRESS` | On | Enable ISA chime mute (HW4) |
| `-DEMERGENCY_VEHICLE_DETECTION` | On | Enable emergency vehicle awareness (HW4) |
| `-DENHANCED_AUTOPILOT` | On | Enable Enhanced Autopilot mode |
| `-DNAG_KILLER` | Off | Compile Autosteer Nag Killer (needs X179 bus tap) |
| `-DRUNTIME_HW_SWITCH` | On (S3) | Allow runtime Legacy/HW3/HW4 mode switching |

## API Endpoints

The web server exposes 29 HTTP endpoints. All feature toggles accept `{"enabled": bool}` and return `{"ok": true}`. POST endpoints are rate-limited to 500 ms.

| Method | Path | Description |
|--------|------|-------------|
| GET | `/` | Web UI (single-page HTML) |
| GET | `/api/status` | Full system status JSON (features, CAN stats, logs) |
| GET | `/api/can-live` | Live CAN signals + frame table |
| GET | `/api/smart-offset` | Smart offset rules and current percentage |
| GET | `/api/preheat/status` | Preheat state (active, temps, elapsed) |
| GET | `/api/can-log/status` | CAN monitor buffer status |
| GET | `/api/can-log/dump` | Binary CAN log download (chunked) |
| GET | `/api/drive-data/status` | Drive data recording status |
| GET | `/api/drive-data/csv` | Drive data CSV download |
| POST | `/api/bypass-tlssc` | Toggle Bypass TLSSC |
| POST | `/api/isa-speed-chime-suppress` | Toggle ISA chime suppress |
| POST | `/api/emergency-vehicle-detection` | Toggle emergency vehicle detection |
| POST | `/api/enhanced-autopilot` | Toggle Enhanced Autopilot |
| POST | `/api/nag-killer` | Toggle Autosteer Nag Killer |
| POST | `/api/china-mode` | Toggle China mode |
| POST | `/api/hw-mode` | Switch hardware mode (reboot required) |
| POST | `/api/profile-mode-auto` | Toggle auto speed profile |
| POST | `/api/speed-profile` | Set manual speed profile (0–4) |
| POST | `/api/speed-offset` | Set manual speed offset |
| POST | `/api/smart-offset` | Configure smart offset rules |
| POST | `/api/preheat` | Toggle battery preheat |
| POST | `/api/preheat/config` | Set preheat auto-stop thresholds |
| POST | `/api/wifi` | Save WiFi STA credentials (triggers reboot) |
| POST | `/api/ota` | Upload firmware binary (supports MD5 via header) |
| POST | `/api/enable-print` | Toggle serial console logging |
| POST | `/api/drive-data/toggle` | Start/stop drive data recording |
| POST | `/api/drive-data/clear` | Clear drive data buffer |
| POST | `/api/can-log/start` | Start CAN monitor logging |
| POST | `/api/can-log/stop` | Stop CAN monitor logging |
| POST | `/api/can-log/clear` | Clear CAN monitor buffer |

## NVS Persistence

All feature toggles, speed offset settings, smart offset rules, WiFi credentials, and hardware mode are persisted to ESP32 NVS flash. Settings survive reboots and power cycles. The firmware detects NVS corruption on boot and auto-recovers by erasing and reinitializing to defaults.

**Exception:** Battery preheat is intentionally NOT persisted — it must be manually re-enabled after each reboot for safety.

## Versioning

- The project version is tracked in [`VERSION`](VERSION) using Semantic Versioning.
- Release notes are tracked in [`CHANGELOG.md`](CHANGELOG.md).

## Third-Party Libraries

This project depends on the following open-source libraries. Full license texts are in [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES).

| Library | License | Copyright |
|---------|---------|-----------|
| [espressif/esp-idf](https://github.com/espressif/esp-idf) (TWAI driver) | Apache 2.0 | (c) 2015-2025 Espressif Systems (Shanghai) CO LTD |

## License

This project is licensed under the **GNU General Public License v3.0** — see the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html) for details.
