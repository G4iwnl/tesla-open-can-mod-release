---
sidebar_position: 3
---

# 호환성

## 차량 펌웨어 호환성

:::warning 업데이트 주의
**HW4에서 FSD가 작동하지 않으므로 2026.2.9.x 및 2026.8.6으로 업데이트하지 마세요.**
:::

| 펌웨어 | HW3 | HW4 | 비고 |
|---|---|---|---|
| < 2026.2.9 | FSD v12.4.6 | HW3으로 컴파일 | HW4 차량은 FSD v13을 위해 HW3 핸들러 사용 |
| 2026.2.9.X | FSD v12.4.6 | FSD v14.x | HW4에서 전체 v14 기능 사용 가능 |
| 2026.8.X | FSD v12.4.6 | FSD v12.4.6 | 이 브랜치의 HW4는 여전히 v12/v13 — HW3으로 컴파일 |

## FSD 버전 차이

### FSD v12/13 (HW3 핸들러)
- FSD 활성화 비트
- 속도 프로파일 (3단계)
- Nag 억제

### FSD v14 (HW4 핸들러)
- FSD 활성화 비트
- 속도 프로파일 (5단계)
- Nag 억제
- 향상된 오토파일럿 (선택 사항)
- 실제 스마트 소환 (ASS)
- 긴급 차량 감지 (선택 사항)
- ISA 속도 경고음 억제 (선택 사항)

## 보드 호환성

지원되는 모든 보드는 동일한 공유 펌웨어 로직을 실행합니다. 유일한 차이점은 CAN 드라이버 구현입니다:

| 보드 | PlatformIO 환경 | Arduino 보드 |
|---|---|---|
| Feather RP2040 CAN | `feather_rp2040_can` | `rp2040:rp2040:adafruit_feather_can` |
| Feather M4 CAN Express | `feather_m4_can` | Feather M4 CAN (SAME51) |
| ESP32 + 트랜시버 | `esp32_twai` | ESP32 Dev Module |
| M5Stack Atomic CAN Base | `m5stack-atomic-can-base` | M5Stack-ATOM |
| M5Stack AtomS3 CAN Base | `m5stack-atoms3-can-base` | M5AtomS3 |

## 서드파티 라이브러리

| 라이브러리 | 라이선스 | 사용 대상 |
|---|---|---|
| [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) | MIT | Feather RP2040 CAN |
| [adafruit/Adafruit_CAN](https://github.com/adafruit/Adafruit_CAN) | MIT | Feather M4 CAN Express |
| [espressif/esp-idf](https://github.com/espressif/esp-idf) (TWAI) | Apache 2.0 | ESP32 보드 |
