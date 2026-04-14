---
sidebar_position: 1
---

# Architecture

The project uses an abstract `CanDriver` interface so that all vehicle logic (handlers, bit manipulation, speed profiles) is shared across boards. Only the driver implementation changes per board.

## Project Structure

```
include/
  can_frame_types.h       # Portable CanFrame struct
  can_driver.h            # Abstract CanDriver interface
  can_helpers.h           # setBit, readMuxID, isFSDSelectedInUI, setSpeedProfileV12V13
  handlers.h              # CarManagerBase, LegacyHandler, HW3Handler, HW4Handler
  app.h                   # Shared setup/loop logic for all entry points
  arduino_entrypoint.h    # Shared Arduino setup/loop entry point
  drivers/
    mcp2515_driver.h      # MCP2515 driver (Feather RP2040 CAN)
    same51_driver.h       # CANSAME5x driver (Feather M4 CAN Express)
    twai_driver.h         # ESP32 TWAI driver
    mock_driver.h         # Mock driver for unit tests
RP2040CAN/
  RP2040CAN.ino           # Arduino IDE sketch entry point
  sketch_config.h         # Shared board, vehicle, and feature defines
  src/                    # Shared headers exposed inside the sketch for Arduino IDE
src/
  main.cpp                # PlatformIO entry point
scripts/
  platformio_set_ino_profile.py   # Switch shared board/vehicle/feature defines
  platformio_sync_ino_defines.py  # Sync shared sketch defines into PlatformIO envs
  platformio_native_env.py        # Add macOS native test compiler includes
test/
  test_native_helpers/    # Tests for bit manipulation helpers
  test_native_legacy/     # LegacyHandler tests
  test_native_hw3/        # HW3Handler tests
  test_native_hw4/        # HW4Handler tests
  test_native_twai/       # TWAI filter computation tests
```

## Driver Abstraction

The `CanDriver` interface defines the contract that all board-specific drivers implement:

**What changes per board:**
- **RP2040 CAN:** `mcp2515.h` (autowp) — SPI-based, struct read/write, needs `PIN_CAN_CS`
- **M4 CAN Express:** `Adafruit_CAN` (`CANSAME5x`) — native MCAN peripheral, packet-stream API, needs `PIN_CAN_BOOSTEN`
- **ESP32 TWAI:** ESP-IDF `driver/twai.h` — native TWAI peripheral, FreeRTOS queue-based RX, needs external transceiver and two GPIO pins

**What stays identical across all boards:**
- All handler structs and bit manipulation logic
- Vehicle-specific behavior (FSD enable, nag suppression, speed profiles)
- Serial debug output

## Handler Pattern

Each vehicle variant has its own handler struct:

- `LegacyHandler` — HW3 retrofit with portrait screen
- `HW3Handler` — HW3 with landscape screen
- `HW4Handler` — HW4 with extended features

All handlers inherit from `CarManagerBase` and share the same bit manipulation helpers from `can_helpers.h`.

## Entry Points

The firmware has two entry points that share the same core logic:

- **Arduino IDE:** `RP2040CAN/RP2040CAN.ino` includes `arduino_entrypoint.h`
- **PlatformIO:** `src/main.cpp` includes `app.h`

Both use the same shared headers from `include/`, ensuring identical behavior regardless of build system.
