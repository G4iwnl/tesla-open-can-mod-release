# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.5.1] - 2026-04-15

### Added

- Smart preheat: real-time battery temp/SOC status panel with elapsed timer, configurable auto-stop temperature threshold (-5°C to 25°C), and max duration limit (10-60 min)
- Preheat auto-stop: automatically stops when battery temp reaches target or max duration exceeded
- API: `/api/preheat/status` (GET) and `/api/preheat/config` (POST) endpoints for preheat monitoring and configuration

### Fixed

- Battery status panel: added missing i18n keys to `applyLang()` — labels now correctly translate when switching language
- OTA banner and drive data card: added missing i18n bindings in `applyLang()`

## [1.5.0] - 2026-04-16

### Added

- Battery status monitoring panel: real-time SOC%, voltage, current, and temperature min/max from BMS CAN signals (0x132, 0x292, 0x312) displayed on dashboard
- OTA update protection: automatically pauses all CAN frame modifications and preheat injection when vehicle OTA is detected (otaInProgress > 0 from 0x318), with auto-resume and UI warning banner
- Drive data recording: ring buffer (4096 entries) captures timestamped signal snapshots, with toggle/clear controls and CSV download from System tab
- i18n: added battery, OTA pause, and drive data recording translation keys (zh/en)
- CSS: added yellow bar color variant for battery SOC warning level

## [1.4.0] - 2026-04-15

### Added

- Smart offset: visual rule editor with per-rule speed/percentage inputs, add/remove buttons, and auto-sort
- Auto theme: UI theme automatically follows vehicle headlight state via CAN lightState (0x185) signal
- i18n: added smartEditTitle translation key for smart offset editor modal (zh/en)
- README: added battery preheat feature description
- README: added upstream attribution to Tesla-OPEN-CAN-MOD original project

### Changed

- Default smart offset rules updated to 4 tiers: <=40 -> 50%, 50 -> 30%, 60 -> 20%, 80-120 -> 10%
- Removed manual theme toggle button; theme is now fully automatic (headlights on = dark, off = light)
- README: translated to English, added 2026.2.300 to blocked firmware versions warning

### Fixed

- Web UI: removed literal backslash-n text displayed between language and theme buttons in header bar

## [1.3.0] - 2026-04-14

### Changed

- Removed all non-ESP32 platform support (RP2040, M4, MCP2515, Arduino IDE)
- ESP32/ESP32-S3 TWAI driver only — single driver, simplified codebase
- O(1) bitmask-based CAN ID filtering in handler loop (was O(n) per frame)
- O(1) direct-mapped lookup table for CAN Live signal updates (was O(n) linear search)
- O(1) bitmask for CAN Monitor watched ID check (was O(21) linear search)
- Optimized CanIdEntry struct layout (28→24 bytes, saves 512B RAM at 128 slots)
- Replaced cJSON tree with direct snprintf in /api/status (1 malloc vs ~50 cJSON nodes)
- Cleaned .gitlab-ci.yml: removed RP2040/M4/FeatherWing CI jobs, added Waveshare + LilyGo jobs
- Updated all docs-site pages to ESP32-only content
- Updated README supported boards table to ESP32-only

### Removed

- MCP2515 and SAME51 CAN drivers
- RP2040CAN/ Arduino IDE sketch folder
- Arduino IDE flashing documentation and CI job
- platformio_set_ino_profile.py and platformio_sync_ino_defines.py scripts
- Feather RP2040, M4, and V2 FeatherWing hardware documentation
- sketch_config.h shared configuration file

## [1.2.3] - 2026-04-14

### Added

- Web UI: light/dark theme toggle with localStorage persistence

### Changed

- Web UI: optimized polling with visibility API (pause when tab hidden)
- Web UI: batch log insertion via DocumentFragment for smoother rendering
- Web UI: skip redundant DOM writes on feature toggles (dirty-check before update)
- Web UI: added CSS containment and GPU acceleration hints for smoother animations
- Web UI: reduced CAN live poll interval from 500ms to 750ms to lower CPU load
- README: added Speed Offset feature description
- README: added Waveshare ESP32-S3 RS485/CAN to supported boards
- README: added HW4 speed offset rows to CAN signal table
- README: updated CI section and PlatformIO build/flash examples

## [1.2.2] - 2026-04-14

### Added

- HW4Handler: added speed offset support (read from mux 0, inject in mux 2), matching HW3 behavior

## [1.1.0] - 2026-04-06

### Added

- Added m5stack-atoms3-mini-can-base as new ESP32 board
- Added enhanced autopilot to enable summon related features and surpress some nags

### Fixed

- HW3Handler: removed obsolete speed-offset-to-profile mapping that overwrote stalk-derived `speedProfile`
- NagHandler: fixed incomplete torque override in echo frame — `data[2]` lower nibble now set to `0x08` to match fixed torque raw value `0x08B6` (1.80 Nm)
- Fixed webui with the new features

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
