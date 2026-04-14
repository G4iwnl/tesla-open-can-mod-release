---
sidebar_position: 3
---

# Testing

The project uses [PlatformIO](https://platformio.org/) with the [Unity](https://github.com/ThrowTheSwitch/Unity) test framework. All tests run on your host machine — no hardware required.

## Running Tests

```bash
# Main test suite — all handler logic
pio test -e native

# BYPASS_TLSSC_REQUIREMENT path tests
pio test -e native_bypass_tlssc_requirement

# Log buffer tests
pio test -e native_log_buffer
```

## What's Tested

Tests cover all handler logic including:

- FSD activation and state reading
- Nag suppression
- Speed profile mapping and injection
- Bit manipulation correctness
- TWAI filter computation
- Log buffer behavior

## Test Structure

```
test/
  test_native_helpers/    # Tests for bit manipulation helpers
  test_native_legacy/     # LegacyHandler tests
  test_native_hw3/        # HW3Handler tests
  test_native_hw4/        # HW4Handler tests
  test_native_twai/       # TWAI filter computation tests
```

Tests use the `MockDriver` from `include/drivers/mock_driver.h` to simulate CAN bus communication without real hardware.

## Continuous Integration

GitLab CI validates three layers on every commit:

1. **Unit tests:** `pio test -e native`, `native_bypass_tlssc_requirement`, `native_log_buffer`
2. **Cross-compilation:** `pio run` for all board environments (RP2040, M4, ESP32, M5Stack)
3. **Arduino IDE compatibility:** `arduino-cli compile` for the RP2040 sketch

The CI pipeline in `.gitlab-ci.yml` uses `scripts/platformio_set_ino_profile.py` to set the correct board/vehicle defines per job.

## Prerequisites for Local Testing

On Windows, you need MinGW-w64 GCC for native test compilation:

```bash
winget install BrechtSanders.WinLibs.POSIX.UCRT
```

Restart your terminal after installation so `gcc` and `g++` are on PATH.

:::tip
On macOS, the native PlatformIO test environments automatically add the Xcode Command Line Tools libc++ headers. If `xcode-select -p` fails, install the Command Line Tools first.
:::
