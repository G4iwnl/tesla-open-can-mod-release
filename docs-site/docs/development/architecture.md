---
sidebar_position: 1
---

# Architecture

The project targets ESP32 and ESP32-S3 boards exclusively, using the native TWAI (CAN) peripheral. All vehicle logic (handlers, bit manipulation, speed profiles) is shared across all supported boards.

## Project Structure

```
include/
  can_frame_types.h       # Portable CanFrame struct
  can_driver.h            # Abstract CanDriver interface
  can_helpers.h           # setBit, readMuxID, isFSDSelectedInUI, setSpeedProfileV12V13
  handlers.h              # CarManagerBase, LegacyHandler, HW3Handler, HW4Handler
  app.h                   # Shared setup/loop logic
  drivers/
    twai_driver.h         # ESP32 TWAI driver
    mock_driver.h         # Mock driver for unit tests
  web/
    web_server.h          # WiFi AP + async HTTP server
    web_ui.h              # Single-page Web UI (HTML/CSS/JS)
src/
  main.cpp                # PlatformIO entry point
scripts/
  platformio_native_env.py        # Add macOS native test compiler includes
test/
  test_native_helpers/    # Tests for bit manipulation helpers
  test_native_legacy/     # LegacyHandler tests
  test_native_hw3/        # HW3Handler tests
  test_native_hw4/        # HW4Handler tests
  test_native_twai/       # TWAI filter computation tests
```

## Driver

All boards use the ESP-IDF TWAI driver (`driver/twai.h`). The `TWAIDriver` class implements the `CanDriver` interface, providing FreeRTOS queue-based RX and two configurable GPIO pins (TX/RX).

## Handler Pattern

Each vehicle variant has its own handler struct:

- `LegacyHandler` — HW3 retrofit with portrait screen
- `HW3Handler` — HW3 with landscape screen
- `HW4Handler` — HW4 with extended features

All handlers inherit from `CarManagerBase` and share the same bit manipulation helpers from `can_helpers.h`.

## Entry Point

`src/main.cpp` is the single entry point. It includes `app.h` which contains the shared setup/loop logic, and `drivers/twai_driver.h` for CAN communication.
