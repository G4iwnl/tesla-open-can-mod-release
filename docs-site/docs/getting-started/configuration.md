---
sidebar_position: 4
---

# 설정

설정은 컴파일 시 옵션을 위한 `platformio.ini`의 `build_flags`와 런타임에 웹 UI를 통해 이루어집니다.

## 차량 하드웨어 선택

PlatformIO 환경의 `-D` 빌드 플래그를 통해 차량 하드웨어 변형을 설정하세요:

```ini
build_flags =
  -D HW4
```

| 정의 | 대상 | 수신하는 CAN ID | 비고 |
|---|---|---|---|
| `LEGACY` | HW3 레트로핏 | 1006, 69 | 추종 거리를 통한 FSD 활성화 비트 및 속도 프로파일 제어 설정 |
| `HW3` | HW3 차량 | 1016, 1021 | Legacy와 동일한 기능 |
| `HW4` | HW4 차량 | 1016, 1021 | 확장된 속도 프로파일 범위 (5단계) |

:::note
펌웨어 **2026.2.9.X**의 HW4 차량은 **FSD v14**를 사용합니다. **2026.8.X** 브랜치의 버전은 여전히 **FSD v13**을 사용합니다. 차량이 FSD v13을 사용하는 경우, HW4 하드웨어가 있더라도 `HW3`으로 컴파일하세요.
:::

## 선택적 기능

`build_flags`에 선택적 기능 플래그를 추가하세요:

```ini
build_flags =
  -D HW4
  -D EMERGENCY_VEHICLE_DETECTION
  -D ENHANCED_AUTOPILOT
```

| 기능 | 설명 |
|---|---|
| `ISA_SPEED_CHIME_SUPPRESS` | ISA 속도 경고음을 억제합니다; 주행 중 속도 제한 표지판이 비어 있게 됩니다 |
| `EMERGENCY_VEHICLE_DETECTION` | 접근하는 긴급 차량 감지를 활성화합니다 (HW4 전용) |
| `BYPASS_TLSSC_REQUIREMENT` | "신호등 및 정지 표지판 제어"를 켜지 않고 항상 FSD를 활성화합니다 |
| `ENHANCED_AUTOPILOT` | HW3/HW4에서 UI_applyEceR79 오버라이드를 활성화하고, HW4에서 소환 지원을 추가합니다 |

## 시리얼 디버그 출력

`enablePrint`가 `true`일 때 디버그 출력이 **115200 보드율**로 시리얼을 통해 출력됩니다. 시리얼 모니터를 열어 실시간 FSD 상태와 활성 속도 프로파일을 확인하세요.
