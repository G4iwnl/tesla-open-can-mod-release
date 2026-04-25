# Autosteer Nag Killer

Tesla 차량에서 오토파일럿의 "핸들에 손을 올리세요" 경고를 억제합니다. 중국산 TSL6P 상용 모듈 (~200 EUR)과 동일한 방법을 약 15 EUR ESP32 보드로 구현한 것입니다.

## 테스트된 차량

| 차량 | HW | 오토파일럿 | 상태 |
|---------|-----|-----------|--------|
| Model Y Performance 2022 | HW3 | 기본 오토파일럿 | **작동 중** |

## 작동 원리

모듈은 CAN 880 (EPAS3P_sysStatus) 프레임을 수정된 페이로드로 에코합니다:

1. `handsOnLevel = 0` (드라이버 손 감지 안 됨)인 CAN 880 프레임 수신 대기
2. 즉시 수정된 프레임 재전송:
   - `byte 3 = 0xB6` → 고정 torsionBarTorque = 1.80 Nm
   - `byte 4 |= 0x40` → handsOnLevel = 1
   - `byte 6 counter + 1` → 다음 예상 카운터 값
   - `byte 7` → 재계산된 체크섬: `(sum(byte0..byte6) + 0x73) & 0xFF`
3. 동일한 카운터의 실제 EPAS 프레임이 우리 프레임 이후 도착 → 수신기가 중복으로 거부
4. 드라이버가 실제 조향 토크를 가하면 (handsOn >= 1), 모듈은 아무것도 하지 않음

### 프레임 예시

```
실제 EPAS:       12 00 08 23 1F 89 4C A4   (ho=0, torque=+0.33Nm, cnt=C)
수정된 버전:     12 00 08 B6 5F 89 4D 78   (ho=1, torque=+1.80Nm, cnt=D)
```

## 하드웨어

CAN 트랜시버가 있는 **ESP32 보드라면 무엇이든** 작동합니다. 테스트된 보드:

- **LILYGO T-CAN485** (~15 EUR) — ESP32 + SN65HVD230 CAN 트랜시버, 스크루 터미널

기타 호환 보드:
- ESP32 + MCP2551 또는 SN65HVD230 브레이크아웃
- M5Stack ATOM CAN
- 이 프로젝트에서 지원하는 모든 보드

## 배선

**X179 진단 커넥터 핀 2와 핀 3**에 연결 (CAN 버스 4):

```
보드 CAN_H  ──────  X179 핀 2  (CAN 버스 4+)
보드 CAN_L  ──────  X179 핀 3  (CAN 버스 4-)
보드 USB    ──────  임의의 5V 소스 (차량 USB, 휴대폰 충전기, 노트북)
```

### X179 20핀 커넥터 핀아웃 (S/X 신형 블루 / Model Y)

```
         ┌─────────────────────────────┐
         │  1   2   3   4   5   6   7  │  ← 윗줄 (핀 1 = 12V+)
         │  8   9  10  11  12  13  14  │  ← 중간줄
         │ 15  16  17  18  19  20      │  ← 아랫줄 (핀 20 = GND)
         └─────────────────────────────┘
                (전면 체결면)
```

| 핀 | 기능 | 비고 |
|-----|----------|-------|
| 1 | 12V+ | 차량 전원 |
| **2** | **CAN 버스 4+** | **← CAN_H를 여기 연결** |
| **3** | **CAN 버스 4-** | **← CAN_L을 여기 연결** |
| 6 | K/시리얼 | |
| 9 | CAN 버스 2+ | 차량 CAN |
| 10 | CAN 버스 2- | 차량 CAN |
| 13 | CAN 버스 6+ | 바디/좌측 CAN (FSD 모드 / Enhance 케이블용) |
| 14 | CAN 버스 6- | 바디/좌측 CAN |
| 18 | CAN 버스 3+ | 섀시 CAN |
| 19 | CAN 버스 3- | 섀시 CAN |
| 20 | 접지 | |

> **중요**: Nag killer는 CAN 버스 4 (핀 2/3)에서만 작동합니다. Tesla의 스푸핑 방지로 인해 CAN 버스 3 (핀 18/19) 또는 CAN 버스 6 (핀 13/14)에서는 작동하지 않습니다.

> **핀 번호 주의**: 항상 커넥터의 전면 체결면에서 핀 번호를 읽으세요. 암 커넥터의 뒷면(전선 쪽)을 보면 번호가 미러 이미지로 나타납니다!

## 빌드

### PlatformIO 사용 (권장)

LILYGO T-CAN485:
```bash
pio run -e lilygo_tcan485_nag_killer
pio run -e lilygo_tcan485_nag_killer -t upload
```

일반 ESP32 + CAN 트랜시버 (기본 핀 GPIO 5/4):
```bash
pio run -e esp32_twai_nag_killer
pio run -e esp32_twai_nag_killer -t upload
```

### Arduino IDE / CLI 사용

빌드 플래그 `-DNAG_KILLER`를 사용하고 TX/RX 핀을 `-DTWAI_TX_PIN=GPIO_NUM_xx -DTWAI_RX_PIN=GPIO_NUM_xx`로 설정하세요.

### LILYGO T-CAN485 핀 매핑

| 기능 | GPIO |
|----------|------|
| CAN TX | 27 |
| CAN RX | 26 |
| 5V Enable | 16 |
| CAN Standby | 23 (LOW = 활성) |

## 작동하지 않는 방법 (광범위하게 테스트됨)

| 방법 | 버스 | 실패 이유 |
|----------|-----|-------------|
| CAN 880 에코 | 버스 3 (섀시, 핀 18/19) | 게이트웨이 스푸핑 방지 차단 |
| CAN 880 에코 | 버스 6 (바디, 핀 13/14) | DAS가 주입된 프레임 무시 |
| CAN 82 에코 | 버스 6 (바디, 핀 13/14) | DAS 무시 |
| CAN 905 에코 | 버스 3 (섀시) | 소스 검증에서 거부 |
| CAN 1160 LKA 토크 | 버스 3 (섀시) | DAS가 50Hz로 전송하여 덮어씀 |
| CAN 82 주입 | 버스 3 (섀시) | 효과 없음 |

**CAN 버스 4 (핀 2/3)만 작동합니다** — 다른 버스와 같은 카운터/소스 검증이 없습니다.

## 안전

- 모듈은 `handsOnLevel = 0`일 때만 활성화됩니다
- 정상적으로 핸들을 잡으면 모듈은 아무것도 하지 않습니다
- CAN 버스-오프 자동 복구가 내장되어 있습니다
- 차량 수정 없음 — 완전히 되돌릴 수 있으며 플러그 앤 플레이
- **항상 핸들 근처에 손을 두고 주의를 기울이세요**

## 크레딧

- BatteryPlug (nicolozak)가 Claude AI의 도움으로 리버스 엔지니어링
- Tesla Open CAN Mod Discord 커뮤니티의 도움으로 TSL6P 방법 해독
- Tesla 커뮤니티(Poppyseed, Onyx)의 DBC 파일
