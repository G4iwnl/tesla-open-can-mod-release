# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.3] - 2026-04-15

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
