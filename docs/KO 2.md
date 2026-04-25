# Tesla FSD CAN 신호 완전 참고 매뉴얼
> 整理日期：2026-04-09
> 数据来源：joshwardell/model3dbc, commaai/opendbc, onyx-m2/onyx-m2-dbc, mikegapinski/tesla-can-explorer, talas9/tesla_can_signals, tuncasoftbildik/tesla-can-mod, flipper-tesla-fsd

---

## 목차

1. [버스 설명](#1-버스-설명)
2. [핵심 제어 프레임 (쓰기/수정 가능)](#2-핵심-제어-프레임쓰기수정-가능)
3. [0x3FD (1021) UI_autopilotControl 프레임 상세 설명](#3-0x3fd-1021-ui_autopilotcontrol-프레임-상세-설명)
4. [0x3EE (1006) DAS_autopilot Legacy 프레임 상세 설명](#4-0x3ee-1006-das_autopilot-legacy-프레임-상세-설명)
5. [속도 제한 인식 관련 신호 (읽기 전용)](#5-속도-제한-인식-관련-신호읽기-전용)
6. [속도 오프셋 및 속도 설정 신호](#6-속도-오프셋-및-속도-설정-신호)
7. [ISA 속도 제한 알림음 억제](#7-isa-속도-제한-알림음-억제)
8. [Nag 억제 (스티어링 휠 힘 감지)](#8-nag-억제스티어링-휠-힘-감지)
9. [차량 간 거리 조절杆 → 속도 기어 맵핑](#9-차량-간-거리-조절杆--속도-기어-맵핑)
10. [하드웨어 검사 및 안전 신호](#10-하드웨어-검사-및-안전-신호)
11. [BMS 배터리 모니터링 (읽기 전용)](#11-bms-배터리-모니터링읽기-전용)
12. [배터리 예열 트리거](#12-배터리-예열-트리거)
13. [트랙 모드 제어](#13-트랙-모드-제어)
14. [조향 보조 제어](#14-조향-보조-제어)
15. [0x3FD 프레임 74개 신호 전체 목록](#15-0x3fd-프레임-74개-신호-전체-목록)
16. [HW3 vs HW4 차이 대조표](#16-hw3-vs-hw4-차이-대조표)
17. [Model 3/Y vs Model S/X CAN ID 차이](#17-model-3y-vs-model-sx-can-id-차이)
18. [DBC 데이터 출처](#18-dbc-데이터-출처)
19. [현재 프로젝트 구현 상태 및 할 일](#19-현재-프로젝트-구현-상태-및-할-일)
20. [Checksum 계산 방법](#20-checksum-계산-방법)

---

## 1. 버스 설명

Tesla 내부 CAN 버스 명명 규칙:

| Tesla 버스 이름 | 설명 | 접속 지점 |
|----------------|------|-----------|
| **ETH** | 고대역폭 CAN-FD 메인(진짜 이더넷 아님) | OBD-II 포트, X179 커넥터 |
| **CH** | 섀시 CAN — EPAS, EPB, ESP, 제동 시스템 | X179 핀 2/3 (CAN 버스 4) |
| **PT** | 파워트레인 CAN — DI, BMS, 충전기 | — |
| **BDY** | 차체 CAN — VCFRONT, VCLEFT, VCRIGHT, 문, 창문, 좌석 | — |
| **OBDII** | 표준 OBD-II 진단 | OBD-II 포트 |

본 프로젝트는 OBD-II / X179를 통해 **ETH** 버스에 접속합니다. FSD 관련 모든 제어 프레임은 이 버스에 있습니다.
Nag Killer에서 사용하는 EPAS 프레임 (0x370)은 **CH** 버스에 있습니다.

---

## 2. 핵심 제어 프레임 (쓰기/수정 가능)

| CAN ID (dec) | CAN ID (hex) | 프레임 이름 | 기능 | 버스 | DLC | 주기 |
|---------------|--------------|-------------|------|------|-----|------|
| **1021** | **0x3FD** | `UI_autopilotControl` | FSD 주 제어 프레임 (HW3/HW4), 다중화, 74개 신호 | ETH | 8 | — |
| **1006** | **0x3EE** | `DAS_autopilot` | FSD 제어 (Legacy HW1/HW2) | ETH | 8 | — |
| **921** | **0x399** | `DAS_status` | ISA 속도 제한 알림음 억제 (HW4), 26개 신호 | ETH | 8 | 500ms |
| **880** | **0x370** | `EPAS3P_sysStatus` | Nag 억제 목표 프레임 (counter+1 echo 방법) | **CH** | 8 | — |
| **787** | **0x313** | `UI_trackModeSettings` | 트랙 모드 요청, 10개 신호 | ETH | 8 | — |
| **130** | **0x082** | `UI_tripPlanning` | 배터리 예열/예조정 트리거 | ETH | 8 | 500ms |

---

## 3. 0x3FD (1021) `UI_autopilotControl` 프레임 상세 설명

이것은 FSD의 가장 핵심적인 프레임입니다. **다중화** 인코딩을 사용하며, 바이트 0의 비트 0-2는 mux ID입니다.
펌웨어에서 이 프레임이 **74개 신호**를 포함하고 있음을 보여줍니다.

### 3.1 Mux 0 — FSD 활성화 + 상태 읽기

#### 알려진 비트 위치 (경험적으로 검증됨)

| 기능 | 비트 위치 | 바이트/비트 | 폭 | HW3 | HW4 | Legacy | 신호 이름 (추정) |
|------|-----------|-------------|------|-----|-----|--------|------------------|
| Mux ID | 비트 0-2 | `data[0] & 0x07` | 3 비트 | 읽기 | 읽기 | 읽기 | — |
| FSD UI 선택 감지 | 비트 54 | `(data[4] >> 6) & 0x01` | 1 비트 | 읽기 | 읽기 | 읽기 | `UI_fsdStopsControlEnabled` |
| **

```
原始位置：byte 3, bit 1-6（帧内 bit 25-30）
提取方法：raw = (data[3] >> 1) & 0x3F
物理值计算：offset = clamp((raw - 30) * 5, 0, 100)
含义：0 = 无偏移, 100 = 最大偏移
```

코드 예제 (C++)：
```cpp
speedOffset = std::max(std::min(((uint8_t)((frame.data[3] >> 1) & 0x3F) - 30) * 5, 100), 0);
```

#### 속도 단계 기록 디코딩 (HW3/Legacy)

```
位置：byte 6, bit 1-2
写入方法：data[6] = (data[6] & ~0x06) | ((profile & 0x03) << 1)
范围：0 = Chill, 1 = Normal, 2 = Max (Hurry)
```

### 3.2 Mux 1 — Nag 억제（ECE R79）

| 기능 | Bit 위치 | 바이트/비트 | HW3 | HW4 | 레거시 | 신호명（추정） |
|------|---------|---------|-----|-----|--------|--------------|
| ECE R79 Nag 억제 | bit 19 | `data[2] bit 3` | **0 쓰기** | **0 쓰기** | **0 쓰기** | `UI_applyEceR79` |
| Enhanced Autopilot 확인 | bit 46 | `data[5] bit 6` | 1 쓰기 | — | — | — |
| HW4 Nag 확인 | bit 47 | `data[5] bit 7` | — | **1 쓰기** | — | — |

### 3.3 Mux 2 — 속도 오프셋 주입 / 속도 기어 주입

#### HW3：속도 오프셋 주입

```
位置：byte 0 bit 6-7 + byte 1 bit 0-5（帧内 bit 6-13，跨两字节，共 8 bit）
写入方法：
  data[0] = (data[0] & ~0xC0) | ((speedOffset & 0x03) << 6)   // 低 2 bit
  data[1] = (data[1] & ~0x3F) | (speedOffset >> 2)             // 高 6 bit
范围：0-100
条件：仅当 FSDEnabled == true 时注入
```

코드 예제 (C++)：
```cpp
frame.data[0] &= ~(0b11000000);
frame.data[1] &= ~(0b00111111);
frame.data[0] |= (speedOffset & 0x03) << 6;
frame.data[1] |= (speedOffset >> 2);
```

#### HW4：속도 단계 주입

```
位置：byte 7 bit 4-6（帧内 bit 60-62）
写入方法：
  data[7] = (data[7] & ~(0x07 << 4)) | ((speedProfile & 0x07) << 4)
范围：0 = Chill, 1 = Normal, 2 = Hurry, 3 = Max, 4 = Sloth
条件：无条件注入（不需要 FSDEnabled）
```

> **注意**：tuncasoftbildik 的实现用的是 `<< 5`（bit 61-63），我们和 flipper 项目用的是 `<< 4`（bit 60-62）。差一位，需要经验验证。

코드 예제 (C++)：
```cpp
frame.data[7] &= ~(0x07 << 4);
frame.data[7] |= (speedProfile & 0x07) << 4;
```

#### HW4 Mux 2의 다른 알려진 신호 (펌웨어 추출에서)

| 신호 인덱스 | 신호 이름 | Bit 위치 | 상태 |
|---------|--------|---------|------|
| #68 | `UI_enableApproachingEmergencyVehicleDetection` | **알 수 없음** | — |
| #69 | `UI_enableStartFsdFromParkBrakeConfirmation` | **알 수 없음** | — |
| #70 | `UI_enableStartFsdFromPark` | **알 수 없음** | — |
| **#71** | **`UI_fsdMaxSpeedOffsetPercentage`** | **알 수 없음** | **역공학 필요** |
| #72 | `UI_coldStartMonarchInFactory` | **알 수 없음** | — |
| #73 | `UI_autopilotControlMux2Valid` | **알 수 없음** | — |

> `UI_fsdMaxSpeedOffsetPercentage` 是 HW4 mux 2 中唯一与速度偏移相关的信号，但目前无任何公开 DBC 定义其 bit 位置。需要 CAN 抓包逆向。

---

## 4. 0x3EE (1006) `DAS_autopilot` 레거시 프레임 상세 설명

레거시(HW1/HW2)는 CAN ID 1006을 사용하여 1021을 대체하며, 프레임 구조는 유사합니다:

| 기능 | Bit/Byte | 설명 |
|------|---------|------|
| Mux ID | `data[0] & 0x07` | 0x3FD와 동일 |
| FSD UI 선택 | `(data[4] >> 6) & 0x01` | 0x3FD와 동일 |
| FSD 활성화 (mux 0) | bit 46 | 1로 설정 |
| 속도 단계 (mux 0) | byte 6 bit 1-2 | HW3와 동일 (0-2) |
| Nag 억제 (mux 1) | bit 19 | 0으로 설정 |

레거시 프레임**에는** mux 2 속도 오프셋 주입이 없습니다.

---

## 5. 속도 제한 인식 관련 신호(읽기 전용)

### 5.1 DAS_fusedSpeedLimit — 융합 속도 제한(가장 정확하며, 추천)

```
CAN ID：0x399 (921) — DAS_status
总线：ETH (Chassis Bus), Party Bus 上为 0x39B (923)
起始 bit：8
宽度：5 bit
字节序：Little Endian
Scale：5
Offset：0
物理范围：0 ~ 150 kph/mph
单位：kph 或 mph（取决于车辆区域设置）
周期：500ms
特殊值：raw 0 = UNKNOWN_SNA, raw 31 = NONE
```

해독 방법:
```cpp
uint8_t raw = (data[1]) & 0x1F;
float speed_limit = raw * 5.0f;  // kph/mph
```

> sunnypilot 实测确认此信号与 Tesla 屏幕上显示的限速完全一致。

### 5.2 DAS_visionOnlySpeedLimit — 시각 인식 한정 속도

```
CAN ID：0x399 (921) — DAS_status
起始 bit：16
宽度：5 bit
Scale：5
Offset：0
物理范围：0 ~ 150 kph/mph
特殊值：raw 0 = UNKNOWN_SNA, raw 31 = NONE
```

해독 방법:
```cpp
uint8_t raw = (data[2]) & 0x1F;
float vision_limit = raw * 5.0f;
```

> sunnypilot 测试发现此信号常固定在 155 kph，不随实际路况变化，准确度不如 fusedSpeedLimit。

### 5.3 UI_mapSpeedLimit — 지도 제한 속도 (열거형)

```
CAN ID：0x238 (568) — UI_driverAssistMapData（Model 3/Y）
        0x3C8 (968) — Model S/X
起始 bit：8
宽度：5 bit
Scale：1
Offset：0
物理范围：0 ~ 31（枚举值）
周期：500ms
```

# 열거형 매핑:
```
 0 = UNKNOWN
 1 = LESS_OR_EQ_5      2 = LESS_OR_EQ_7      3 = LESS_OR_EQ_10
 4 = LESS_OR_EQ_15     5 = LESS_OR_EQ_20     6 = LESS_OR_EQ_25
 7 = LESS_OR_EQ_30     8 = LESS_OR_EQ_35     9 = LESS_OR_EQ_40
10 = LESS_OR_EQ_45    11 = LESS_OR_EQ_50    12 = LESS_OR_EQ_55
13 = LESS_OR_EQ_60    14 = LESS_OR_EQ_65    15 = LESS_OR_EQ_70
16 = LESS_OR_EQ_75    17 = LESS_OR_EQ_80    18 = LESS_OR_EQ_85
19 = LESS_OR_EQ_90    20 = LESS_OR_EQ_95    21 = LESS_OR_EQ_100
22 = LESS_OR_EQ_105   23 = LESS_OR_EQ_110   24 = LESS_OR_EQ_115
25 = LESS_OR_EQ_120   26 = LESS_OR_EQ_130   27 = LESS_OR_EQ_140
28 = LESS_OR_EQ_150   29 = LESS_OR_EQ_160   30 = UNLIMITED
31 = SNA
```

동일 프레임 연관 신호:
```
UI_mapSpeedLimitDependency  (bit 0, 3 bit): NONE/SCHOOL/RAIN/SNOW/TIME/SEASON/LANE/SNA
UI_mapSpeedLimitType        (bit 13, 3 bit): REGULAR/ADVISORY/DEPENDENT/BUMPS/UNKNOWN_SNA
UI_mapSpeedUnits            (bit 7, 1 bit):  0=MPH, 1=KPH
```

### 5.4 UI_mppSpeedLimit — MPP 처리 후 속도 제한

```
CAN ID：0x3D9 (985) — UI_gpsVehicleSpeed（Model 3/Y）
        0x2F8 (760) — Model S/X
起始 bit：48
宽度：5 bit
Scale：5
Offset：0
物理范围：0 ~ 155 kph/mph
周期：1000ms
```

### 5.5 UI_conditionalSpeedLimit — 조건부 속도 제한

```
CAN ID：0x3D9 (985) — UI_gpsVehicleSpeed（Model 3/Y）
起始 bit：56
宽度：5 bit
Scale：5
Offset：0
物理范围：0 ~ 155 kph/mph
特殊值：raw 31 = SNA
关联信号：UI_conditionalLimitActive (bit 55, 1 bit)
```

### 5.6 UI_baseMapSpeedLimitMPS — 기본 지도 제한 속도 (m/s 단위)

```
CAN ID：0x218 (536) — UI_driverAssistRoadSign（Model 3/Y，multiplexed, mux index = 3）
        0x238 (568) — Model S/X
起始 bit：8
宽度：8 bit
Scale：0.25
Offset：0
物理范围：0 ~ 63.75 m/s
特殊值：raw 255 = SNA
```

### 5.7 DAS_suppressSpeedWarning — 과속 경고 표시 억제

```
CAN ID：0x399 (921) — DAS_status
起始 bit：13
宽度：1 bit
值：0 = Do_Not_Suppress, 1 = Suppress_Speed_Warning
```

---

## 6. 속도 오프셋 및 속도 설정 신호

### 6.1 UI_userSpeedOffset — 사용자 UI 설정 속도 오프셋

이는 사용자가 Tesla UI에서 설정한 Autopilot 속도 오프셋 값(+1에서 +10 등)입니다.

```
CAN ID：0x3D9 (985) — UI_gpsVehicleSpeed（Model 3/Y）
        0x2F8 (760) — Model S/X
起始 bit：40
宽度：6 bit
字节序：Little Endian (@1+)
Scale：1
Offset：-30
物理范围：-30 ~ +33 kph/mph
单位：kph 或 mph（由 UI_userSpeedOffsetUnits 决定）
```

DBC 정의:
```
SG_ UI_userSpeedOffset : 40|6@1+ (1,-30) [-30|33] "kph/mph" DAS
```

해독 방법:
```cpp
uint8_t raw = (data[5]) & 0x3F;
int offset = (int)raw - 30;  // 物理值，单位 kph 或 mph
```

연관 신호:
```
UI_userSpeedOffsetUnits   (bit 47, 1 bit): 0=MPH, 1=KPH
UI_mapSpeedLimitUnits     (bit 46, 1 bit): 0=MPH, 1=KPH
```

### 6.2 UI_smartSetSpeedOffset — 0x3FD mux 0의 속도 오프셋

이것은 0x3FD 프레임 mux 0의 속도 오프셋 신호로, 현재 HW3Handler가 사용하고 있습니다.

```
CAN ID：0x3FD (1021) — UI_autopilotControl, Mux 0
位置：byte 3, bit 1-6（帧内 bit 25-30）
宽度：6 bit
编码方式：raw = physical + 30
```

해독 방법(현재 프로젝트 구현):
```cpp
int raw = (int)((frame.data[3] >> 1) & 0x3F) - 30;
int offset = clamp(raw * 5, 0, 100);
```

关联 신호(펌웨어 추출에서, 공개되지 않은 bit 위치):
```
UI_smartSetSpeedOffsetType: 枚举 FIXED_OFFSET=0, PERCENTAGE_OFFSET=1
UI_automaticSetSpeedOffset: 自动速度偏移
UI_smartSetSpeed:           智能设定速度
```

### 6.3 UI_fsdMaxSpeedOffsetPercentage — FSD 최대 속도 오프셋 백분율

```
CAN ID：0x3FD (1021) — UI_autopilotControl, Mux 2
信号索引：#71（固件提取）
Bit 位置：未知 ← 需要 CAN 抓包逆向
宽度：未知
说明：控制 FSD 的速度上限百分比。固件中存在此信号，但无任何公开 DBC 定义其 bit 位置。
```

### 6.4 DAS_accSpeedLimit — ACC/자율주행 속도 제한

```
CAN ID：0x389 (905) — DAS_status2
起始 bit：0
宽度：10 bit
Scale：0.2（joshwardell/onyx-m2）或 0.4（commaai party bus）
Offset：0
物理范围：0 ~ 204.6 mph
特殊值：raw 0 = NONE, raw 1023 = SNA
周期：500ms
```

# 해독 방법:
```cpp
uint16_t raw = ((uint16_t)(data[1] & 0x03) << 8) | data[0];
float acc_limit_mph = raw * 0.2f;
```

### 6.5 DAS_setSpeed — DAS가 목표 속도를 설정합니다

```
CAN ID：0x2B9 (697) — DAS_control
起始 bit：0
宽度：12 bit
Scale：0.1
Offset：0
物理范围：0 ~ 409.4 kph
特殊值：raw 4095 = SNA
周期：40ms
```

# 해독 방법:
```cpp
uint16_t raw = ((uint16_t)(data[1] & 0x0F) << 8) | data[0];
float set_speed_kph = raw * 0.1f;
```

### 6.6 UI_speedLimit — 전체 차량 속도 제한 장치

```
CAN ID：0x334 (820) — UI_powertrainControl（Model 3/Y）
        0x313 (787) — Model S/X
起始 bit：16
宽度：8 bit
Scale：1
Offset：50
物理范围：50 ~ 285 kph
特殊值：raw 255 = SNA
周期：500ms
```

# 해독 방법:
```cpp
float vmax_kph = (float)data[2] + 50.0f;
```

### 6.7 DI_cruiseSetSpeed — DI 크루즈 설정 속도

```
CAN ID：0x286 (646) — DI_locStatus
起始 bit：15
宽度：9 bit
Scale：0.5
Offset：0
关联信号：DI_cruiseSetSpeedUnits (bit 24, 1 bit): 0=MPH, 1=KPH
```

### 6.8 UI_speedLimitTick — 속도 제한 미세 조정

```
CAN ID：0x213 (531) — UI_cruiseControl
起始 bit：0
宽度：8 bit
Scale：1
特殊值：raw 255 = SNA
周期：500ms
来源：仅 onyx-m2/onyx-m2-dbc
```

---

## 7. ISA 제한 속도 알림음 억제

### CAN ID: 0x399 (921) — DAS_status

**HW4에서만 유효합니다.** byte 1의 bit 5를 설정하여 ISA 제한 속도 알림음을 억제합니다.

수정 비트:
```
byte 1, bit 5 → 写 1（data[1] |= 0x20）
```

체크섬(바이트 7)을 다시 계산해야 합니다:
```
checksum = sum(byte[0]..byte[6]) + (CAN_ID & 0xFF) + (CAN_ID >> 8)
         = sum(byte[0]..byte[6]) + 0x99 + 0x03
byte[7] = checksum & 0xFF
```

코드 예제 (C++)：
```cpp
frame.data[1] |= 0x20;
uint8_t sum = 0;
for (int i = 0; i < 7; i++)
    sum += frame.data[i];
sum += (921 & 0xFF) + (921 >> 8);  // 0x99 + 0x03
frame.data[7] = sum & 0xFF;
```

### DAS_status 프레임의 다른 신호 (CAN 0x399)

| 신호명 | Bit | 폭 | 설명 |
|--------|-----|------|------|
| `DAS_autopilotState` | — | — | 자동 운전 상태 |
| `DAS_fusedSpeedLimit` | 8 | 5 bit | 융합 속도 제한 (×5 kph) |
| `DAS_suppressSpeedWarning` | 13 | 1 bit | 과속 경고 억제 |
| `DAS_visionOnlySpeedLimit` | 16 | 5 bit | 시각적 속도 제한 (×5 kph) |
| `DAS_blindSpotRearLeft` | — | — | 왼쪽 후방 사각지대 |
| `DAS_blindSpotRearRight` | — | — | 오른쪽 후방 사각지대 |
| `DAS_summonAvailable` | — | — | 소환 가능 |

---

## 8. Nag 억제 (스티어링 휠 힘 감지)

### 방법 1: 0x3FD mux 1 ECE R79 비트 초기화

3.2 절 참조. 비트 19 = 0으로 설정 (HW4 추가 설정 비트 47 = 1).

### 방법 2: CAN 880 (0x370) 카운터 +1 에코

**버스: CH (섀시 CAN), 다른 물리적 접속 지점 필요.**

원리: EPAS3P_sysStatus 프레임을 모니터링하고, `handsOnLevel == 0` (손이 감지되지 않음, 즉 nag이 발생할 예정)일 때 가짜 프레임을 구성:
1. 원본 프레임 복사
2. `handsOnLevel = 1`로 설정 (바이트 4 비트 6-7 → 01)
3. `torsionBarTorque = 0xB6`로 설정 (고정 토크 1.80 Nm)
4. 카운터 +1 (바이트 6 하위 4 비트)
5. 체크섬 재계산 (바이트 7)

가짜 프레임이 실제 프레임보다 먼저 도착하며, 실제 프레임은 카운터가 동일하여 중복 프레임으로 간주되어 폐기됩니다.

```
CAN ID：0x370 (880) — EPAS3P_sysStatus
总线：CH (Chassis CAN)

读取字段：
  handsOnLevel: (data[4] >> 6) & 0x03  — 0=无手, 1=轻握, 2/3=握紧
  counter:      data[6] & 0x0F

构造 Echo 帧：
  echo.data[0] = frame.data[0]
  echo.data[1] = frame.data[1]
  echo.data[2] = (frame.data[2] & 0xF0) | 0x08   // torque quality nibble
  echo.data[3] = 0xB6                              // torsionBarTorque = 1.80 Nm
  echo.data[4] = frame.data[4] | 0x40              // handsOnLevel = 1
  echo.data[5] = frame.data[5]
  echo.data[6] = (frame.data[6] & 0xF0) | ((counter + 1) & 0x0F)
  echo.data[7] = checksum

Checksum: sum(byte[0]..byte[6]) + 0x70 + 0x03
```

코드 예제 (C++)：
```cpp
uint8_t cnt = (frame.data[6] & 0x0F);
cnt = (cnt + 1) & 0x0F;
echo.data[6] = (frame.data[6] & 0xF0) | cnt;

uint16_t sum = 0;
for (int i = 0; i < 7; i++)
    sum += echo.data[i];
sum += (0x370 & 0xFF) + (0x370 >> 8);  // 0x70 + 0x03
echo.data[7] = (uint8_t)(sum & 0xFF);
```

> 已测试：Model Y Performance 2022 HW3, Basic Autopilot, X179 pin 2/3 (CAN bus 4)

---

## 9. 차량 거리 막대 → 속도 기어 매핑

### 9.1 HW3/HW4 — CAN 1016 (0x3F8) `UI_driverAssistControl`

```
信号：UI_accFollowDistanceSetting
位置：byte 5, bit 5-7（帧内 bit 40-42）
宽度：3 bit
提取方法：fd = (data[5] & 0xE0) >> 5
范围：1-7
```

#### HW3 매핑 (3 단계)

| 추적 거리 fd | 속도 단계 | 이름 |
|------------|---------|------|
| 1 | 2 | 최대 (서두르기) |
| 2 | 1 | 정상 |
| 3 | 0 | 느긋하게 |

#### HW4 매핑 (5 단계) — 현재 프로젝트 사용자 정의

| 추적 거리 fd | 속도 단계 | 이름 |
|------------|---------|------|
| 1, 2, 3 | 3 | 최대 |
| 4 | 2 | 서두르기 |
| 5 | 1 | 정상 |
| 6 | 0 | 느긋하게 |
| 7 | 4 | 느림보 |

> 当前项目将 fd 1-3 统一映射到 Max，因为物理杆的最小位置通常被限制（clamped），确保最激进的档位可达。

#### HW4 매핑 — flipper 프로젝트 원본

| 차량 거리 fd | 속도 단계 | 이름 |
|------------|---------|------|
| 1 | 3 | 최대 |
| 2 | 2 | 서두르기 |
| 3 | 1 | 일반 |
| 4 | 0 | 느긋하게 |
| 5 | 4 | 게으름 |

### 9.2 레거시 — CAN 69 (0x045) `STW_ACTN_RQ`

```
信号：方向盘杆位置
位置：byte 1, bit 5-7
宽度：3 bit
提取方法：pos = data[1] >> 5
```

映射：
| pos | 속도 단계 | 이름 |
|-----|---------|------|
| 0, 1 | 2 | 최대 (서두르기) |
| 2 | 1 | 일반 |
| 3+ | 0 | 느긋하게 |

---

## 10. 하드웨어 감지 및 안전 신호

### 10.1 GTW_carConfig — 하드웨어 버전 감지

```
CAN ID：0x398 (920) — GTW_carConfig
字段：DAS_HWversion
位置：byte 0, bit 6-7
宽度：2 bit
提取方法：das_hw = (data[0] >> 6) & 0x03
```

| 원시 값 | 하드웨어 버전 |
|--------|---------|
| 0 | 알 수 없음 |
| 1 | 알 수 없음 (구형?) |
| 2 | **HW3** (FSD 컴퓨터) |
| 3 | **HW4** (AI4 / HW4D) |

### 10.2 GTW_carState — OTA 업데이트 감지

```
CAN ID：0x318 (792) — GTW_carState
字段：GTW_updateInProgress
位置：byte 6, bit 0-1（帧内 bit 48-49）
宽度：2 bit
提取方法：in_progress = (data[6]) & 0x03
```

**in_progress가 0이 아닐 때, 모든 CAN TX를 중단해야 합니다**. OTA 업데이트에 방해가 되지 않도록 합니다.

---

## 11. BMS 배터리 모니터링 (읽기 전용)

### 11.1 BMS_hvBusStatus — 배터리 전압/전류

```
CAN ID：0x132 (306)
DLC：8

电压：byte 0-1, uint16 LE, × 0.01 V
  voltage = ((data[1] << 8) | data[0]) * 0.01

电流：byte 2-3, int16 LE (有符号), × 0.1 A
  current = (int16_t)((data[3] << 8) | data[2]) * 0.1

功率：voltage * current (W)
```

### 11.2 BMS_socStatus — 배터리 SOC

```
CAN ID：0x292 (658)

SOC：byte 0-1 低 10 bit, × 0.1%
  soc = (((data[1] & 0x03) << 8) | data[0]) * 0.1
```

### 11.3 BMS_thermalStatus — 배터리 온도

```
CAN ID：0x312 (786)

最低温度：byte 4 - 40 (°C)
  temp_min = data[4] - 40

最高温度：byte 5 - 40 (°C)
  temp_max = data[5] - 40
```

---

## 12. 배터리 예열 트리거

```
CAN ID：0x082 (130) — UI_tripPlanning
DLC：8
周期：500ms（持续发送直到关闭）

构造方法：
  memset(data, 0, 8)
  data[0] = 0x05   // bit 0 = tripPlanningActive, bit 2 = requestActiveBatteryHeating

效果：让 BMS 认为有导航到超充的路线在活动，触发电池+座舱预调节。
      可在任何位置手动触发，不需要真正设置超充目的地。
```

---

## 13. 트랙 모드 제어

```
CAN ID：0x313 (787) — UI_trackModeSettings (DLS_steeringControl)
总线：ETH
信号数：10

已知信号：
  UI_stabilityModeRequest
  UI_trackModeRequest      — bit 0-1, 2 bit
  UI_trackStabilityAssist
  UI_trackRotationTendency

写入方法：
  data[0] = (data[0] & ~0x03) | (request & 0x03)
  data[7] = computeTeslaChecksum(frame)  // 需要重算 checksum

当前项目 HW3Handler 中实现了 Track Mode 请求（kTrackModeRequestOn = 0x01）。
```

---

## 14. 전환 지원 제어

```
CAN ID：0x101 (257) — GTW_epasControl
总线：CH (Chassis CAN)

信号（来自 tuncasoftbildik TESLA_CAN_STEERING_REFERENCE.md）：
  GTW_epasTuneRequest (bit 2, 3 bit):
    0 = FAIL_SAFE
    1 = DM_COMFORT
    2 = DM_STANDARD
    3 = DM_SPORT
    4 = RWD_COMFORT
    5 = RWD_STANDARD
    6 = RWD_SPORT

  GTW_epasPowerMode (bit 6, 4 bit):
    0 = DRIVE_OFF
    1 = DRIVE_ON
    ...

  GTW_epasLDWEnabled (bit 12, 1 bit):
    0 = DISABLE
    1 = ENABLE

状态：可通过注入此帧改变 Comfort/Standard/Sport 转向手感。
      HW4/Juniper 上未经公开验证。
```

---

## 15. 0x3FD 프레임 74 개 신호 전체 목록

다음은 Tesla MCU 펌웨어 (libQtCarVAPI.so)에서 추출한 `UI_autopilotControl` 프레임의 모든 신호 이름입니다.
**신호 이름과 열거형 값만 있으며, 비트 위치는 포함되어 있지 않습니다.** 비트 위치는 경험적 역공학에서 가져온 것입니다 (출처 표시).

```
 #0  (mux ID)
 #1  UI_autosteerActivation
 #2  UI_apmv3Branch
 #3  UI_enableFullSelfDriving              ← bit 46 (经验验证)
 #4  UI_hasFullSelfDriving
 #5  UI_fullSelfDrivingSuspended
 #6  UI_fsdStopsControlEnabled
 #7  UI_fsdContinueOnGreenWithCIPV
 #8  UI_fsdBetaRequest
 #9  UI_smartSetSpeedOffset                ← bit 25-30 (经验验证, HW3)
#10  UI_smartSetSpeedOffsetType            ← FIXED_OFFSET=0, PERCENTAGE_OFFSET=1
#11  UI_applyEceR79                        ← bit 19 (经验验证)
#12  UI_apply2021_1958_ISA
#13  UI_apply2021_646_ELKS
#14  UI_apply2021_1341_DDAW
#15  UI_automaticSetSpeedOffset
#16  UI_smartSetSpeed
#17  UI_applyR152_AEBS
#18  UI_autoTurnSignalMode
#19  UI_factorySummonEnable
#20  UI_hardCoreSummon
#21  UI_enableFullSelfDriving (duplicate?)
#22  UI_fsdVisualizationEnabled
#23  UI_autopilotDrivingProfile            ← byte 6 bit 1-2 (HW3), byte 7 bit 4-6 (HW4)
#24  ...
...
#68  UI_enableApproachingEmergencyVehicleDetection  ← bit 59 (HW4, 经验验证)
#69  UI_enableStartFsdFromParkBrakeConfirmation
#70  UI_enableStartFsdFromPark
#71  UI_fsdMaxSpeedOffsetPercentage        ← mux 2, bit 位置未知
#72  UI_coldStartMonarchInFactory
#73  UI_autopilotControlMux2Valid
```

> 完整的 74 个信号列表见 mikegapinski/tesla-can-explorer 的 `can_frames_decoded_all_values_mcu3.json` 文件。

---

## 16. HW3 vs HW4 차이 대조표

### 기능 지원 차이

| 기능 | HW3 | HW4 | Legacy |
|------|-----|-----|--------|
| FSD 활성화 (bit 46) | 예 | 예 | 예 |
| FSD V14 이차 잠금 (bit 60) | — | 예 | — |
| 긴급 차량 감지 (bit 59) | — | 선택적 | — |
| ECE R79 Nag (bit 19) | 0 쓰기 | 0 쓰기 | 0 쓰기 |
| HW4 Nag 확인 (bit 47) | — | 1 쓰기 | — |
| 속도 오프셋 읽기 (mux 0) | 예 | **아니요** | — |
| 속도 오프셋 주입 (mux 2) | 예 (bit 6-13) | **아니요** | — |
| 속도 기어 (mux 0) | 예 (2 bit, 0-2) | — | 예 (2 bit, 0-2) |
| 속도 기어 (mux 2) | — | 예 (3 bit, 0-4) | — |
| ISA 경고음 억제 (0x399) | — | 예 | — |
| 트랙 모드 (0x313) | 예 | — | — |

### CAN ID 사용 차이

| CAN ID | HW3 | HW4 | Legacy |
|--------|-----|-----|--------|
| 0x3FD (1021) | 주 제어 프레임 | 주 제어 프레임 | — |
| 0x3EE (1006) | — | — | 주 제어 프레임 |
| 0x399 (921) | — | ISA 억제 | — |
| 0x3F8 (1016) | 추적 거리 | 추적 거리 | — |
| 0x045 (69) | — | — | 막대 위치 |
| 0x313 (787) | 트랙 모드 | — | — |
| 0x370 (880) | Nag 킬러 | Nag 킬러 | Nag 킬러 |

### 속도 기어 수준 차이

| 수준 | 값 | HW3 | HW4 |
|------|---|-----|-----|
| Chill | 0 | 예 | 예 |
| Normal | 1 | 예 | 예 |
| Hurry | 2 | 예 (= 최대) | 예 |
| Max | 3 | — | 예 |
| Sloth | 4 | — | 예 |

---

## 17. Model 3/Y vs Model S/X CAN ID 차이

| 신호 그룹 | Model 3/Y | Model S/X |
|--------|-----------|-----------|
| DAS_status (fusedSpeedLimit 등) | 0x399 (921) 섀시 / 0x39B (923) 파티 | 0x399 (921) |
| DAS_status2 (accSpeedLimit) | 0x389 (905) | 0x389 (905) |
| DAS_control (setSpeed) | 0x2B9 (697) | 0x2B9 (697) |
| UI_driverAssistMapData (mapSpeedLimit) | 0x238 (568) | 0x3C8 (968) |
| UI_gpsVehicleSpeed (userSpeedOffset) | **0x3D9 (985)** | **0x2F8 (760)** |
| UI_powertrainControl (speedLimit) | 0x334 (820) | 0x313 (787) |
| UI_driverAssistRoadSign | 0x218 (536) | 0x238 (568) |
| UI_cruiseControl (speedLimitTick) | 0x213 (531) | — |

---

## 18. DBC 데이터 출처

| 출처 | 유형 | 커버리지 | URL |
|------|------|------|-----|
| **joshwardell/model3dbc** | DBC | Model 3/Y, bit 위치 완전 | https://github.com/joshwardell/model3dbc |
| **commaai/opendbc** | DBC | Model S/X + Model 3/Y (party/vehicle bus) | https://github.com/commaai/opendbc |
| **onyx-m2/onyx-m2-dbc** | DBC | Model 3, 가장 완전한 열거 값 | https://github.com/onyx-m2/onyx-m2-dbc |
| **mikegapinski/tesla-can-explorer** | JSON | 전 모델, 577 프레임 40484 신호 (bit 위치 없음) | https://github.com/mikegapinski/tesla-can-explorer |
| **talas9/tesla_can_signals** | JSON | 전 모델, bit 위치 있음 (단 UI/DAS 프레임 없음) | https://github.com/talas9/tesla_can_signals |
| **tuncasoftbildik/tesla-can-mod** | C++ | ESP32-C6, 경험적 bit 위치 | https://github.com/tuncasoftbildik/tesla-can-mod |
| **flipper-tesla-fsd** | C | Flipper Zero + ESP32, 경험적 검증 | 로컬 참조 프로젝트 |
| **sunnypilot/opendbc PR #308** | DBC 패치 | 제한 속도 신호 검증 | https://github.com/sunnypilot/opendbc/pull/308 |
| **BYDcar/opendbc-byd** | DBC 포크 | Model S/X, UI_userSpeedOffset 완전 정의 포함 | https://github.com/BYDcar/opendbc-byd |

### 교차 참조 방법

```
1. mikegapinski → 查信号名 + 帧地址 + 总线名
2. talas9       → 查 bit 位置 + 宽度（如果信号存在）
3. opendbc      → 查 DBC 定义（较旧但 bit 准确）
4. joshwardell  → 查 Model 3/Y 专用 DBC
5. 逆向代码    → 查经验 bit 位置（tuncasoftbildik, flipper-tesla-fsd）
```

---

## 19. 현재 프로젝트 구현 상태 및 할 일

### 구현 완료

| 기능 | 핸들러 | 상태 |
|------|---------|------|
| FSD 활성화 (bit 46) | HW3, HW4, Legacy | 완료 |
| FSD V14 이중 잠금 (bit 60) | HW4 | 완료 |
| 긴급 차량 감지 (bit 59) | HW4 | 완료 (런타임 토글) |
| ECE R79 경고 (bit 19) | HW3, HW4, Legacy | 완료 |
| HW4 경고 확인 (bit 47) | HW4 | 완료 |
| 속도 오프셋 읽기/주입 | HW3 (mux 0 → mux 2) | 완료 |
| 속도 기어 (HW3, mux 0) | HW3, Legacy | 완료 |
| 속도 기어 (HW4, mux 2) | HW4 | 완료 |
| ISA 알림음 억제 (0x399) | HW4 | 완료 (런타임 토글) |
| 트랙 모드 (0x313) | HW3 | 완료 |
| 경고 제거기 (0x370 echo) | NagHandler | 완료 (컴파일 타임) |
| 배터리 예열 (0x082) | app.h 메인 루프 | 완료 (런타임 토글) |
| 추적 거리 → 기어 매핑 | HW3, HW4, Legacy | 완료 |

### 미구현 / 할 일

| 기능 | 이유 | 난이도 |
|------|------|------|
| **HW4 속도 오프셋 읽기/주입** | mux 0의 오프셋 bit가 HW4에서 검증되지 않음; mux 2의 `UI_fsdMaxSpeedOffsetPercentage` bit 미확인 | CAN 패킷 캡처 필요 |
| **속도 제한 인식 리스너** (DAS_fusedSpeedLimit) | CAN 0x399가 HW4 필터에 포함되어 있으며, 읽기 전용 해석 추가 가능 | 낮음 |
| **사용자 속도 오프셋 리스너** (UI_userSpeedOffset) | CAN 0x3D9 필터 추가 필요 | 낮음 |
| **ACC 속도 상한 리스너** (DAS_accSpeedLimit) | CAN 0x389 필터 추가 필요 | 낮음 |
| **DAS 목표 속도 리스너** (DAS_setSpeed) | CAN 0x2B9 필터 추가 필요 | 낮음 |
| **조향 보조 제어** (GTW_epasTuneRequest) | CAN 0x101이 CH 버스에 있으며, 다른 물리적 연결 필요 | 중간 |

---

## 20. 체크섬 계산 방법

Tesla는 통일된 체크섬 알고리즘을 사용합니다 (0x399, 0x370, 0x313 등 프레임에 사용):

```
checksum = sum(byte[0] .. byte[N-2]) + (CAN_ID & 0xFF) + (CAN_ID >> 8)
byte[N-1] = checksum & 0xFF   // 最后一个字节为 checksum
```

其中 N = DLC（보통 8），checksum 바이트는 보통 byte[7]입니다.

코드 예제（C++，현재 프로젝트 `computeTeslaChecksum` 함수）：
```cpp
uint8_t computeTeslaChecksum(const CanFrame &frame, uint8_t checksumByteIndex = 7)
{
    uint16_t sum = (uint16_t)(frame.id & 0xFF) + (uint16_t)((frame.id >> 8) & 0xFF);
    for (uint8_t i = 0; i < frame.dlc; ++i) {
        if (i == checksumByteIndex) continue;
        sum += frame.data[i];
    }
    return (uint8_t)(sum & 0xFF);
}
```

각 프레임의 CAN ID 분해:

| CAN ID | Hex | Low byte | High byte | 상수 및 |
|--------|-----|----------|-----------|--------|
| 921 | 0x399 | 0x99 | 0x03 | 0x9C |
| 880 | 0x370 | 0x70 | 0x03 | 0x73 |
| 787 | 0x313 | 0x13 | 0x03 | 0x16 |

---

*문서 종료. 마지막 업데이트: 2026-04-09*
