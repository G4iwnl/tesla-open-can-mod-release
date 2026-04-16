---
sidebar_position: 99
---

# Changelog

All notable changes to Tesla Open CAN Mod are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

## [1.6.1] - 2026-04-16

### Performance

- **canLiveHandler**: Replaced cJSON tree (~64 malloc/free per poll) with zero-allocation snprintf serialization
- **statusHandler**: Static 8KB buffer eliminates per-call heap allocation
- **decodeSignals**: Deduplicated 3→1 calls per app loop iteration
- **Canvas chart**: requestAnimationFrame throttling, skip redraw when Dashboard tab not visible
- **CAN frame table**: requestAnimationFrame-batched DOM updates
- **DOM queries**: Cached querySelectorAll results for repeated selectors
- **Poll interval**: can-live reduced from 750ms to 1000ms
- **sortById() removed**: Eliminated O(n²) insertion sort on every can-live poll

## [1.6.0] - 2026-04-16

### Added

- **NVS Persistence**: Smart offset rules and manual speed offset saved to NVS flash
- **Real-time Chart**: Live speed/SOC/power Canvas chart on Dashboard
- **CAN Frame Viewer**: Interactive CAN frame table with filtering and statistics
- **System Info Panel**: Chip temperature, handler latency, free heap/PSRAM
- **Handler Latency Tracking**: Microsecond-precision timing of CAN handler execution
- **ESP32 Chip Temperature**: Internal temperature sensor monitoring
- **OTA MD5 Verification**: Firmware upload integrity check
- **NVS Corruption Recovery**: Enhanced NVS init with erase-on-corruption fallback

## [1.5.1] - 2026-04-16

### Added

- Smart preheat: battery temp/SOC status panel with auto-stop thresholds
- API: `/api/preheat/status` and `/api/preheat/config` endpoints

### Fixed

- Battery status panel: missing i18n keys in `applyLang()`
- OTA banner and drive data card: missing i18n bindings

## [1.5.0] - 2026-04-15

### Added

- Battery status monitoring panel (SOC%, voltage, current, temperature)
- OTA update protection: pauses CAN modifications during vehicle OTA
- Drive data recording with CSV export

## [1.4.0] - 2026-04-15

### Added

- Smart offset: visual rule editor with speed-based rules
- Auto theme: follows vehicle headlight state

### Changed

- Default smart offset rules updated to 4 tiers
- Removed manual theme toggle (now fully automatic)

## [1.3.0] - 2026-04-14

### Changed

- Removed all non-ESP32 platform support (RP2040, M4, MCP2515, Arduino IDE)
- ESP32/ESP32-S3 TWAI driver only
- O(1) bitmask CAN filtering, O(1) signal lookup, O(1) monitor check

### Removed

- MCP2515 and SAME51 CAN drivers
- RP2040CAN/ Arduino IDE sketch folder
- sketch_config.h configuration file

## [1.2.3] - 2026-04-14

### Added

- Web UI: light/dark theme toggle

### Changed

- Web UI: polling with visibility API, batch log insertion, CSS optimizations

## [1.2.2] - 2026-04-14

### Added

- HW4Handler: speed offset support

## [1.1.0] - 2026-04-06

### Added

- Added m5stack-atoms3-mini-can-base as new ESP32 board
- Added enhanced autopilot to enable summon related features

### Fixed

- HW3Handler: removed obsolete speed-offset-to-profile mapping
- NagHandler: fixed incomplete torque override in echo frame

## [1.0.0] - 2026-04-05

### Added

- FSD activation bypass for HW3 and HW4 vehicles
- `BYPASS_TLSSC_REQUIREMENT` build flag to bypass Tesla Live Service SC requirement for regions without traffic light toggle
- Autosteer nag suppression via CAN frame interception
- Autosteer Nag Killer hardware mode: echoes CAN frame 0x370 with counter+1 to suppress nag at hardware level (X179 connector, CAN bus 4)
- ISA speed chime suppression for HW3 and HW4
- Emergency vehicle detection and response
- Speed profiles support (distance control stalk mapped)
- Smart Summon support (EU region restriction removed)
- ESP32 web dashboard for live CAN status and runtime settings
- OTA firmware updates via web interface for ESP32 boards
- Hardware support: Adafruit Feather RP2040 CAN
- Hardware support: Adafruit Feather M4 CAN Express (tested)
- Hardware support: ESP32 with TWAI driver
- Hardware support: Adafruit Feather ESP32 V2 with MCP2515 CAN Featherwing
- Hardware support: M5Stack Atomic CAN Base
- Hardware support: LILYGO T-CAN485
- CAN driver abstraction layer (MCP2515, SAME51, TWAI)
- TWAI driver: non-blocking TX, bus-off cooldown and recovery, driver-fail guard
- TWAI driver: DLC clamped to 8 bytes on read and send
- DLC validation guards on all CAN frame handlers
- Bounds check in `setBit()` to prevent buffer overrun
- STL case model for Feather RP2040 CAN
- FSD subscription guide for unsupported regions (Canadian account method)
- Wiring guide for Tesla Model 3/Y including legacy connector pinouts
- Comprehensive NagHandler unit test suite

### Fixed

- Nag handler torque value: output is now fixed at safe 1.80 Nm (0x08B6) instead of copying torque from the original frame
- FSDEnabled variable shadowing bug in HW3 and HW4 handlers
- TWAI TX timeout changed from 0 ms to 2 ms to avoid bus starvation

### Changed

- Build flag renamed from `FORCE_FSD` / `FORCE_FSC` to `BYPASS_TLSSC_REQUIREMENT` for clarity
- Arduino sketch renamed from `canFeather.ino` to `RP2040CAN.ino` for multi-board support
- Firmware configuration consolidated: all user-selectable options moved to `sketch_config.h`
