# Contributing

Thanks for your interest in contributing to Tesla Open Can Mod!

## Quick Start

```bash
# Clone
git clone https://github.com/ylovex75/tesla-open-can-mod-release.git
cd tesla-open-can-mod-release

# Build both targets
pio run -e waveshare_esp32s3_rs485_can -e esp32_twai

# Run tests (114 native tests)
pio test -e native
```

## Development Setup

- **IDE:** VS Code + PlatformIO extension (recommended)
- **Toolchain:** PlatformIO CLI (`pio`)
- **Targets:** `waveshare_esp32s3_rs485_can` (ESP32-S3) and `esp32_twai` (ESP32)
- **Test environment:** `native` (runs on host, no hardware needed)

## Code Style

- C++17 (`-std=gnu++17`)
- Indentation: 4 spaces, no tabs
- Braces: same line for functions and control flow
- Header-only architecture: all logic lives in `include/` headers
- Format with `clang-format --style=file` before committing

## Pull Request Process

1. **Fork** the repository and create a branch from `main`
2. **Build** both targets and ensure no errors: `pio run -e waveshare_esp32s3_rs485_can -e esp32_twai`
3. **Test** with `pio test -e native` — all 114+ tests must pass
4. **Update** `CHANGELOG.md` under `[Unreleased]` for user-facing changes
5. **Update** documentation in `docs-site/docs/` if the change affects features or behavior
6. **Submit** the PR with a clear description of what changed and why

### PR Checklist

- [ ] Code compiles for both ESP32 and ESP32-S3 targets
- [ ] `pio test -e native` passes
- [ ] `CHANGELOG.md` updated (if user-facing change)
- [ ] Documentation updated (if feature/behavior change)
- [ ] No new compiler warnings

## What to Contribute

### Good First Issues

- Improve i18n translations (add new languages or fix existing ones)
- Add unit tests for CAN signal decoding
- Documentation improvements

### Feature Contributions

- New CAN signal decoders (read-only monitoring)
- Web UI improvements
- New board support (ESP32-based with TWAI)

### Safety-Critical Contributions

Changes that modify CAN TX behavior (frame injection or modification) require extra scrutiny:

- Clearly document which CAN IDs and bits are affected
- Add corresponding unit tests in `test/`
- Update `SECURITY.md` TX surface table
- Expect a longer review cycle

## Project Structure

```
include/
  app.h              — Main loop, handler dispatch, OTA protection
  handlers.h         — Legacy/HW3/HW4/Nag CAN frame handlers
  can_live.h         — Real-time signal decoding & O(1) lookup
  can_helpers.h      — Tesla checksum, bit manipulation
  can_monitor.h      — PSRAM-backed CAN frame logger
  log_buffer.h       — Ring buffer for system log messages
  shared_types.h     — Shared enums and structs
  drivers/
    twai_driver.h    — ESP32 TWAI CAN driver
  web/
    web_server.h     — HTTP server, NVS, WiFi, API endpoints
    web_ui.h         — Single-page web dashboard (HTML/CSS/JS)
src/
  main.cpp           — Entry point (calls app loop)
test/
  test_native_*/     — Native unit tests (no hardware)
```

## Versioning

- Version tracked in `VERSION` (semver) and `include/version.h`
- Both files must be updated together on version bumps
- `CHANGELOG.md` follows [Keep a Changelog](https://keepachangelog.com/)

## Reporting Bugs

Use the [issue tracker](https://github.com/ylovex75/tesla-open-can-mod-release/issues). For security issues, see [SECURITY.md](SECURITY.md).

## License

By contributing, you agree that your contributions will be licensed under GPL-3.0.
