---
sidebar_position: 1
---

# 하드웨어 선택

## 지원 보드

| 보드 | CAN 인터페이스 | 상태 |
|---|---|---|
| [Waveshare ESP32-S3 RS485/CAN](https://www.waveshare.com/esp32-s3-relay-6ch.htm) | ESP32-S3 TWAI를 통한 내장 SIT1051T 트랜시버 | 권장 |
| ESP32 + CAN 트랜시버 (예: ESP32-DevKitC + SN65HVD230) | 네이티브 TWAI 주변장치 | 테스트됨 |
| [M5Stack Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | ESP32 TWAI를 통한 CA-IS3050G | 테스트됨 |
| M5Stack AtomS3 + CAN Base | ESP32-S3 TWAI를 통한 CA-IS3050G | 테스트됨 |

## 지원 차량 변형

`platformio.ini`의 `build_flags` 또는 런타임 웹 UI를 통해 차량 하드웨어 변형을 선택하세요.

| 정의 | 대상 | 수신하는 CAN ID | 비고 |
|---|---|---|---|
| `LEGACY` | HW3 레트로핏 | 1006, 69 | 추종 거리를 통한 FSD 활성화 비트 및 속도 프로파일 제어 설정 |
| `HW3` | HW3 차량 | 1016, 1021 | Legacy와 동일한 기능 |
| `HW4` | HW4 차량 | 1016, 1021 | 확장된 속도 프로파일 범위 (5단계) |

:::note
펌웨어 **2026.2.9.X**의 HW4 차량은 **FSD v14**를 사용합니다. 그러나 **2026.8.X** 브랜치의 버전은 여전히 **FSD v13**을 사용합니다. 차량이 FSD v13 (2026.8.X 브랜치 또는 2026.2.9보다 오래된 것 포함)을 사용하는 경우, HW4 하드웨어가 있더라도 `HW3`으로 컴파일하세요.
:::

## 하드웨어 변형 확인 방법

- **Legacy** — 차량에 **세로형 센터 스크린**과 **HW3**이 있습니다. 이는 원래 HW3이 장착되어 있거나 HW3으로 업그레이드된 구형 (팔라디움 이전) Model S 및 Model X 차량에 해당합니다.
- **HW3** — 차량에 **가로형 센터 스크린**과 **HW3**이 있습니다. 차량 터치스크린에서 **제어 > 소프트웨어 > 추가 차량 정보**에서 하드웨어 버전을 확인할 수 있습니다.
- **HW4** — 위와 동일하지만 추가 차량 정보 화면에 **HW4**가 표시됩니다.

## 하드웨어 요구 사항

- 위 지원 보드 중 하나
- 차량에 CAN 버스 연결 (500 kbit/s)

:::important
ESP32 보드 또는 외부 CAN 트랜시버에 120 Ohm 종단 저항이 있는 경우 제거하거나 비활성화하세요. 차량의 CAN 버스에는 이미 자체 종단이 있으며, 두 번째 저항을 추가하면 통신 오류가 발생합니다.
:::

## 어떤 보드를 선택해야 할까요?

- **Waveshare ESP32-S3 RS485/CAN** — 권장. 내장 CAN 트랜시버, WiFi, 웹 UI, 16MB 플래시, 소형
- **M5Stack Atomic CAN Base** — 가장 소형
- **M5Stack AtomS3 + CAN Base** — ESP32-S3를 사용한 소형 구성
- **ESP32 + CAN 트랜시버** — ESP32가 이미 있다면 가장 저렴한 옵션, 외부 트랜시버 모듈 필요
