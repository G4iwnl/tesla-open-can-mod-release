---
sidebar_position: 3
---

# 펌웨어 플래싱

펌웨어는 **PlatformIO**를 사용하여 빌드하고 플래시합니다.

## 사전 조건 (Windows)

| 도구 | 용도 | 설치 |
|------|---------|---------|
| [Python 3](https://www.python.org/downloads/) | PlatformIO 런타임 | `winget install Python.Python.3.14` |
| [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) | 빌드 시스템 & 테스트 실행 | `pip install platformio` |
| [MinGW-w64 GCC](https://winlibs.com/) | 네이티브 테스트 컴파일러 | `winget install BrechtSanders.WinLibs.POSIX.UCRT` |

:::tip
MinGW-w64 설치 후 터미널을 재시작하여 `gcc`와 `g++`이 PATH에 등록되도록 하세요. GCC는 `pio test -e native` (호스트 측 단위 테스트) 에만 필요합니다 — 보드에 크로스 컴파일하는 것은 PlatformIO의 내장 툴체인을 사용합니다.
:::

## 빌드

```bash
# Waveshare ESP32-S3 RS485/CAN (권장)
pio run -e waveshare_esp32s3_rs485_can

# TWAI (CAN) 주변장치가 있는 ESP32
pio run -e esp32_twai

# LilyGo T-CAN485 (HW3)
pio run -e lilygo_tcan485_hw3

# M5Stack Atomic CAN Base
pio run -e m5stack-atomic-can-base

# M5Stack AtomS3 CAN Base
pio run -e m5stack-atoms3-mini-can-base
```

## 플래시

보드를 USB로 연결한 후 업로드:

```bash
# Waveshare ESP32-S3 RS485/CAN (권장)
pio run -e waveshare_esp32s3_rs485_can --target upload

# ESP32
pio run -e esp32_twai --target upload

# LilyGo T-CAN485 (HW3)
pio run -e lilygo_tcan485_hw3 --target upload

# M5Stack Atomic CAN Base
pio run -e m5stack-atomic-can-base --target upload

# M5Stack AtomS3 CAN Base
pio run -e m5stack-atoms3-mini-can-base --target upload
```

:::tip
ESP32 보드에서 자동 리셋이 작동하지 않는 경우 업로드 중 **BOOT** 버튼을 누르고 있으세요.
:::

## 테스트 실행

```bash
pio test -e native
pio test -e native_bypass_tlssc_requirement
pio test -e native_log_buffer
```

테스트는 호스트 머신에서 실행됩니다 — 하드웨어가 필요 없습니다.
