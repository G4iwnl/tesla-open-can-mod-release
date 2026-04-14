# Tesla FSD CAN 信号完整参考手册

> 整理日期：2026-04-09
> 数据来源：joshwardell/model3dbc, commaai/opendbc, onyx-m2/onyx-m2-dbc, mikegapinski/tesla-can-explorer, talas9/tesla_can_signals, tuncasoftbildik/tesla-can-mod, flipper-tesla-fsd

---

## 目录

1. [总线说明](#1-总线说明)
2. [核心控制帧（可写入/修改）](#2-核心控制帧可写入修改)
3. [0x3FD (1021) UI_autopilotControl 帧详解](#3-0x3fd-1021-ui_autopilotcontrol-帧详解)
4. [0x3EE (1006) DAS_autopilot Legacy 帧详解](#4-0x3ee-1006-das_autopilot-legacy-帧详解)
5. [限速识别相关信号（只读）](#5-限速识别相关信号只读)
6. [速度偏移与速度设定信号](#6-速度偏移与速度设定信号)
7. [ISA 限速提示音抑制](#7-isa-限速提示音抑制)
8. [Nag 抑制（方向盘施力检测）](#8-nag-抑制方向盘施力检测)
9. [跟车距离杆 → 速度档位映射](#9-跟车距离杆--速度档位映射)
10. [硬件检测与安全信号](#10-硬件检测与安全信号)
11. [BMS 电池监控（只读）](#11-bms-电池监控只读)
12. [电池预热触发](#12-电池预热触发)
13. [Track Mode 控制](#13-track-mode-控制)
14. [转向助力控制](#14-转向助力控制)
15. [0x3FD 帧 74 个信号完整列表](#15-0x3fd-帧-74-个信号完整列表)
16. [HW3 vs HW4 差异对照表](#16-hw3-vs-hw4-差异对照表)
17. [Model 3/Y vs Model S/X CAN ID 差异](#17-model-3y-vs-model-sx-can-id-差异)
18. [DBC 数据来源](#18-dbc-数据来源)
19. [当前项目实现状态与待办](#19-当前项目实现状态与待办)
20. [Checksum 计算方法](#20-checksum-计算方法)

---

## 1. 总线说明

Tesla 内部 CAN 总线命名规则：

| Tesla 总线名 | 说明 | 接入点 |
|-------------|------|--------|
| **ETH** | 高带宽 CAN-FD 主干（并非真正的以太网）| OBD-II 端口, X179 连接器 |
| **CH** | 底盘 CAN — EPAS, EPB, ESP, 制动系统 | X179 pin 2/3 (CAN bus 4) |
| **PT** | 动力总成 CAN — DI, BMS, 充电器 | — |
| **BDY** | 车身 CAN — VCFRONT, VCLEFT, VCRIGHT, 门窗座椅 | — |
| **OBDII** | 标准 OBD-II 诊断 | OBD-II 端口 |

本项目通过 OBD-II / X179 接入 **ETH** 总线。FSD 相关的所有控制帧均在此总线上。
Nag Killer 使用的 EPAS 帧 (0x370) 在 **CH** 总线上。

---

## 2. 核心控制帧（可写入/修改）

| CAN ID (dec) | CAN ID (hex) | 帧名 | 功能 | 总线 | DLC | 周期 |
|-------------|-------------|------|------|------|-----|------|
| **1021** | **0x3FD** | `UI_autopilotControl` | FSD 主控帧（HW3/HW4），multiplexed，74 个信号 | ETH | 8 | — |
| **1006** | **0x3EE** | `DAS_autopilot` | FSD 控制（Legacy HW1/HW2） | ETH | 8 | — |
| **921** | **0x399** | `DAS_status` | ISA 限速提示音抑制（HW4），26 个信号 | ETH | 8 | 500ms |
| **880** | **0x370** | `EPAS3P_sysStatus` | Nag 抑制目标帧（counter+1 echo 方法） | **CH** | 8 | — |
| **787** | **0x313** | `UI_trackModeSettings` | Track Mode 请求，10 个信号 | ETH | 8 | — |
| **130** | **0x082** | `UI_tripPlanning` | 电池预热/预调节触发 | ETH | 8 | 500ms |

---

## 3. 0x3FD (1021) `UI_autopilotControl` 帧详解

这是 FSD 最核心的帧。使用 **multiplexed** 编码，byte 0 的 bit 0-2 为 mux ID。
固件提取显示该帧包含 **74 个信号**。

### 3.1 Mux 0 — FSD 启用 + 状态读取

#### 已知 bit 位置（经验验证）

| 功能 | Bit 位置 | 字节/位 | 宽度 | HW3 | HW4 | Legacy | 信号名（推测） |
|------|---------|---------|------|-----|-----|--------|--------------|
| Mux ID | bit 0-2 | `data[0] & 0x07` | 3 bit | 读 | 读 | 读 | — |
| FSD UI 选中检测 | bit 54 | `(data[4] >> 6) & 0x01` | 1 bit | 读 | 读 | 读 | `UI_fsdStopsControlEnabled` |
| **FSD 启用** | **bit 46** | `data[5] bit 6` | 1 bit | **写 1** | **写 1** | **写 1** | `UI_enableFullSelfDriving` |
| **FSD V14 二次锁** | **bit 60** | `data[7] bit 4` | 1 bit | — | **写 1** | — | （HW4 专有） |
| 紧急车辆检测 | bit 59 | `data[7] bit 3` | 1 bit | — | 可选写 1 | — | `UI_enableApproachingEmergencyVehicleDetection` |
| **速度偏移读取** | bit 25-30 | `(data[3] >> 1) & 0x3F` | 6 bit | **读取** | **未实现** | — | `UI_smartSetSpeedOffset` |
| **速度档位写入** | bit 49-50 | `data[6] bit 1-2` | 2 bit | **写入** | — | **写入** | `UI_autopilotDrivingProfile` |

#### 速度偏移读取解码（HW3）

```
原始位置：byte 3, bit 1-6（帧内 bit 25-30）
提取方法：raw = (data[3] >> 1) & 0x3F
物理值计算：offset = clamp((raw - 30) * 5, 0, 100)
含义：0 = 无偏移, 100 = 最大偏移
```

代码示例（C++）：
```cpp
speedOffset = std::max(std::min(((uint8_t)((frame.data[3] >> 1) & 0x3F) - 30) * 5, 100), 0);
```

#### 速度档位写入解码（HW3/Legacy）

```
位置：byte 6, bit 1-2
写入方法：data[6] = (data[6] & ~0x06) | ((profile & 0x03) << 1)
范围：0 = Chill, 1 = Normal, 2 = Max (Hurry)
```

### 3.2 Mux 1 — Nag 抑制（ECE R79）

| 功能 | Bit 位置 | 字节/位 | HW3 | HW4 | Legacy | 信号名（推测） |
|------|---------|---------|-----|-----|--------|--------------|
| ECE R79 Nag 抑制 | bit 19 | `data[2] bit 3` | **写 0** | **写 0** | **写 0** | `UI_applyEceR79` |
| Enhanced Autopilot 确认 | bit 46 | `data[5] bit 6` | 写 1 | — | — | — |
| HW4 Nag 确认 | bit 47 | `data[5] bit 7` | — | **写 1** | — | — |

### 3.3 Mux 2 — 速度偏移注入 / 速度档位注入

#### HW3：速度偏移注入

```
位置：byte 0 bit 6-7 + byte 1 bit 0-5（帧内 bit 6-13，跨两字节，共 8 bit）
写入方法：
  data[0] = (data[0] & ~0xC0) | ((speedOffset & 0x03) << 6)   // 低 2 bit
  data[1] = (data[1] & ~0x3F) | (speedOffset >> 2)             // 高 6 bit
范围：0-100
条件：仅当 FSDEnabled == true 时注入
```

代码示例（C++）：
```cpp
frame.data[0] &= ~(0b11000000);
frame.data[1] &= ~(0b00111111);
frame.data[0] |= (speedOffset & 0x03) << 6;
frame.data[1] |= (speedOffset >> 2);
```

#### HW4：速度档位注入

```
位置：byte 7 bit 4-6（帧内 bit 60-62）
写入方法：
  data[7] = (data[7] & ~(0x07 << 4)) | ((speedProfile & 0x07) << 4)
范围：0 = Chill, 1 = Normal, 2 = Hurry, 3 = Max, 4 = Sloth
条件：无条件注入（不需要 FSDEnabled）
```

> **注意**：tuncasoftbildik 的实现用的是 `<< 5`（bit 61-63），我们和 flipper 项目用的是 `<< 4`（bit 60-62）。差一位，需要经验验证。

代码示例（C++）：
```cpp
frame.data[7] &= ~(0x07 << 4);
frame.data[7] |= (speedProfile & 0x07) << 4;
```

#### HW4 Mux 2 中的其他已知信号（来自固件提取）

| 信号索引 | 信号名 | Bit 位置 | 状态 |
|---------|--------|---------|------|
| #68 | `UI_enableApproachingEmergencyVehicleDetection` | **未知** | — |
| #69 | `UI_enableStartFsdFromParkBrakeConfirmation` | **未知** | — |
| #70 | `UI_enableStartFsdFromPark` | **未知** | — |
| **#71** | **`UI_fsdMaxSpeedOffsetPercentage`** | **未知** | **待逆向** |
| #72 | `UI_coldStartMonarchInFactory` | **未知** | — |
| #73 | `UI_autopilotControlMux2Valid` | **未知** | — |

> `UI_fsdMaxSpeedOffsetPercentage` 是 HW4 mux 2 中唯一与速度偏移相关的信号，但目前无任何公开 DBC 定义其 bit 位置。需要 CAN 抓包逆向。

---

## 4. 0x3EE (1006) `DAS_autopilot` Legacy 帧详解

Legacy（HW1/HW2）使用 CAN ID 1006 替代 1021，帧结构类似：

| 功能 | Bit/Byte | 说明 |
|------|---------|------|
| Mux ID | `data[0] & 0x07` | 同 0x3FD |
| FSD UI 选中 | `(data[4] >> 6) & 0x01` | 同 0x3FD |
| FSD 启用 (mux 0) | bit 46 | 写 1 |
| 速度档位 (mux 0) | byte 6 bit 1-2 | 同 HW3 (0-2) |
| Nag 抑制 (mux 1) | bit 19 | 写 0 |

Legacy 帧**没有** mux 2 速度偏移注入。

---

## 5. 限速识别相关信号（只读）

### 5.1 DAS_fusedSpeedLimit — 融合限速（最准确，推荐）

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

解码方法：
```cpp
uint8_t raw = (data[1]) & 0x1F;
float speed_limit = raw * 5.0f;  // kph/mph
```

> sunnypilot 实测确认此信号与 Tesla 屏幕上显示的限速完全一致。

### 5.2 DAS_visionOnlySpeedLimit — 仅视觉识别限速

```
CAN ID：0x399 (921) — DAS_status
起始 bit：16
宽度：5 bit
Scale：5
Offset：0
物理范围：0 ~ 150 kph/mph
特殊值：raw 0 = UNKNOWN_SNA, raw 31 = NONE
```

解码方法：
```cpp
uint8_t raw = (data[2]) & 0x1F;
float vision_limit = raw * 5.0f;
```

> sunnypilot 测试发现此信号常固定在 155 kph，不随实际路况变化，准确度不如 fusedSpeedLimit。

### 5.3 UI_mapSpeedLimit — 地图限速（枚举类型）

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

枚举映射：
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

同帧关联信号：
```
UI_mapSpeedLimitDependency  (bit 0, 3 bit): NONE/SCHOOL/RAIN/SNOW/TIME/SEASON/LANE/SNA
UI_mapSpeedLimitType        (bit 13, 3 bit): REGULAR/ADVISORY/DEPENDENT/BUMPS/UNKNOWN_SNA
UI_mapSpeedUnits            (bit 7, 1 bit):  0=MPH, 1=KPH
```

### 5.4 UI_mppSpeedLimit — MPP 处理后限速

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

### 5.5 UI_conditionalSpeedLimit — 条件性限速

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

### 5.6 UI_baseMapSpeedLimitMPS — 基础地图限速（m/s 单位）

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

### 5.7 DAS_suppressSpeedWarning — 抑制超速警告标志

```
CAN ID：0x399 (921) — DAS_status
起始 bit：13
宽度：1 bit
值：0 = Do_Not_Suppress, 1 = Suppress_Speed_Warning
```

---

## 6. 速度偏移与速度设定信号

### 6.1 UI_userSpeedOffset — 用户 UI 设定的速度偏移

这是用户在 Tesla UI 中设置的 Autopilot 速度偏移值（+1 到 +10 等）。

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

DBC 定义：
```
SG_ UI_userSpeedOffset : 40|6@1+ (1,-30) [-30|33] "kph/mph" DAS
```

解码方法：
```cpp
uint8_t raw = (data[5]) & 0x3F;
int offset = (int)raw - 30;  // 物理值，单位 kph 或 mph
```

关联信号：
```
UI_userSpeedOffsetUnits   (bit 47, 1 bit): 0=MPH, 1=KPH
UI_mapSpeedLimitUnits     (bit 46, 1 bit): 0=MPH, 1=KPH
```

### 6.2 UI_smartSetSpeedOffset — 0x3FD mux 0 中的速度偏移

这是 0x3FD 帧 mux 0 中的速度偏移信号，当前 HW3Handler 正在使用。

```
CAN ID：0x3FD (1021) — UI_autopilotControl, Mux 0
位置：byte 3, bit 1-6（帧内 bit 25-30）
宽度：6 bit
编码方式：raw = physical + 30
```

解码方法（项目当前实现）：
```cpp
int raw = (int)((frame.data[3] >> 1) & 0x3F) - 30;
int offset = clamp(raw * 5, 0, 100);
```

关联信号（来自固件提取，无公开 bit 位）：
```
UI_smartSetSpeedOffsetType: 枚举 FIXED_OFFSET=0, PERCENTAGE_OFFSET=1
UI_automaticSetSpeedOffset: 自动速度偏移
UI_smartSetSpeed:           智能设定速度
```

### 6.3 UI_fsdMaxSpeedOffsetPercentage — FSD 最大速度偏移百分比

```
CAN ID：0x3FD (1021) — UI_autopilotControl, Mux 2
信号索引：#71（固件提取）
Bit 位置：未知 ← 需要 CAN 抓包逆向
宽度：未知
说明：控制 FSD 的速度上限百分比。固件中存在此信号，但无任何公开 DBC 定义其 bit 位置。
```

### 6.4 DAS_accSpeedLimit — ACC/Autopilot 速度上限

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

解码方法：
```cpp
uint16_t raw = ((uint16_t)(data[1] & 0x03) << 8) | data[0];
float acc_limit_mph = raw * 0.2f;
```

### 6.5 DAS_setSpeed — DAS 下发目标速度

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

解码方法：
```cpp
uint16_t raw = ((uint16_t)(data[1] & 0x0F) << 8) | data[0];
float set_speed_kph = raw * 0.1f;
```

### 6.6 UI_speedLimit — 整车速度限制器

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

解码方法：
```cpp
float vmax_kph = (float)data[2] + 50.0f;
```

### 6.7 DI_cruiseSetSpeed — DI 巡航设定速度

```
CAN ID：0x286 (646) — DI_locStatus
起始 bit：15
宽度：9 bit
Scale：0.5
Offset：0
关联信号：DI_cruiseSetSpeedUnits (bit 24, 1 bit): 0=MPH, 1=KPH
```

### 6.8 UI_speedLimitTick — 速度限制微调

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

## 7. ISA 限速提示音抑制

### CAN ID: 0x399 (921) — DAS_status

**仅 HW4 有效。** 通过设置 byte 1 的 bit 5 来抑制 ISA 限速提示音。

修改位：
```
byte 1, bit 5 → 写 1（data[1] |= 0x20）
```

需要重新计算 checksum（byte 7）：
```
checksum = sum(byte[0]..byte[6]) + (CAN_ID & 0xFF) + (CAN_ID >> 8)
         = sum(byte[0]..byte[6]) + 0x99 + 0x03
byte[7] = checksum & 0xFF
```

代码示例（C++）：
```cpp
frame.data[1] |= 0x20;
uint8_t sum = 0;
for (int i = 0; i < 7; i++)
    sum += frame.data[i];
sum += (921 & 0xFF) + (921 >> 8);  // 0x99 + 0x03
frame.data[7] = sum & 0xFF;
```

### DAS_status 帧中的其他信号（CAN 0x399）

| 信号名 | Bit | 宽度 | 说明 |
|--------|-----|------|------|
| `DAS_autopilotState` | — | — | 自动驾驶状态 |
| `DAS_fusedSpeedLimit` | 8 | 5 bit | 融合限速 (×5 kph) |
| `DAS_suppressSpeedWarning` | 13 | 1 bit | 抑制超速警告 |
| `DAS_visionOnlySpeedLimit` | 16 | 5 bit | 仅视觉限速 (×5 kph) |
| `DAS_blindSpotRearLeft` | — | — | 左后盲区 |
| `DAS_blindSpotRearRight` | — | — | 右后盲区 |
| `DAS_summonAvailable` | — | — | 召唤可用 |

---

## 8. Nag 抑制（方向盘施力检测）

### 方法一：0x3FD mux 1 ECE R79 bit 清零

见第 3.2 节。设置 bit 19 = 0（HW4 额外设置 bit 47 = 1）。

### 方法二：CAN 880 (0x370) Counter+1 Echo

**总线：CH（底盘 CAN），需要不同的物理接入点。**

原理：监听 EPAS3P_sysStatus 帧，当 `handsOnLevel == 0`（无手检测到，即将触发 nag）时，构造一个假帧：
1. 复制原始帧
2. 设置 `handsOnLevel = 1`（byte 4 bit 6-7 → 01）
3. 设置 `torsionBarTorque = 0xB6`（固定扭矩 1.80 Nm）
4. Counter +1（byte 6 低 4 bit）
5. 重算 checksum（byte 7）

假帧先于真实帧到达，真实帧因 counter 相同被作为重复帧丢弃。

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

代码示例（C++）：
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

## 9. 跟车距离杆 → 速度档位映射

### 9.1 HW3/HW4 — CAN 1016 (0x3F8) `UI_driverAssistControl`

```
信号：UI_accFollowDistanceSetting
位置：byte 5, bit 5-7（帧内 bit 40-42）
宽度：3 bit
提取方法：fd = (data[5] & 0xE0) >> 5
范围：1-7
```

#### HW3 映射（3 级）

| 跟车距离 fd | 速度档位 | 名称 |
|------------|---------|------|
| 1 | 2 | Max (Hurry) |
| 2 | 1 | Normal |
| 3 | 0 | Chill |

#### HW4 映射（5 级）— 当前项目自定义

| 跟车距离 fd | 速度档位 | 名称 |
|------------|---------|------|
| 1, 2, 3 | 3 | Max |
| 4 | 2 | Hurry |
| 5 | 1 | Normal |
| 6 | 0 | Chill |
| 7 | 4 | Sloth |

> 当前项目将 fd 1-3 统一映射到 Max，因为物理杆的最小位置通常被限制（clamped），确保最激进的档位可达。

#### HW4 映射 — flipper 项目原始

| 跟车距离 fd | 速度档位 | 名称 |
|------------|---------|------|
| 1 | 3 | Max |
| 2 | 2 | Hurry |
| 3 | 1 | Normal |
| 4 | 0 | Chill |
| 5 | 4 | Sloth |

### 9.2 Legacy — CAN 69 (0x045) `STW_ACTN_RQ`

```
信号：方向盘杆位置
位置：byte 1, bit 5-7
宽度：3 bit
提取方法：pos = data[1] >> 5
```

映射：
| pos | 速度档位 | 名称 |
|-----|---------|------|
| 0, 1 | 2 | Max (Hurry) |
| 2 | 1 | Normal |
| 3+ | 0 | Chill |

---

## 10. 硬件检测与安全信号

### 10.1 GTW_carConfig — 硬件版本检测

```
CAN ID：0x398 (920) — GTW_carConfig
字段：DAS_HWversion
位置：byte 0, bit 6-7
宽度：2 bit
提取方法：das_hw = (data[0] >> 6) & 0x03
```

| Raw 值 | 硬件版本 |
|--------|---------|
| 0 | Unknown |
| 1 | Unknown (Legacy?) |
| 2 | **HW3** (FSD Computer) |
| 3 | **HW4** (AI4 / HW4D) |

### 10.2 GTW_carState — OTA 更新检测

```
CAN ID：0x318 (792) — GTW_carState
字段：GTW_updateInProgress
位置：byte 6, bit 0-1（帧内 bit 48-49）
宽度：2 bit
提取方法：in_progress = (data[6]) & 0x03
```

**当 in_progress != 0 时，必须暂停所有 CAN TX**，防止干扰 OTA 更新。

---

## 11. BMS 电池监控（只读）

### 11.1 BMS_hvBusStatus — 包电压/电流

```
CAN ID：0x132 (306)
DLC：8

电压：byte 0-1, uint16 LE, × 0.01 V
  voltage = ((data[1] << 8) | data[0]) * 0.01

电流：byte 2-3, int16 LE (有符号), × 0.1 A
  current = (int16_t)((data[3] << 8) | data[2]) * 0.1

功率：voltage * current (W)
```

### 11.2 BMS_socStatus — 电池 SOC

```
CAN ID：0x292 (658)

SOC：byte 0-1 低 10 bit, × 0.1%
  soc = (((data[1] & 0x03) << 8) | data[0]) * 0.1
```

### 11.3 BMS_thermalStatus — 电池温度

```
CAN ID：0x312 (786)

最低温度：byte 4 - 40 (°C)
  temp_min = data[4] - 40

最高温度：byte 5 - 40 (°C)
  temp_max = data[5] - 40
```

---

## 12. 电池预热触发

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

## 13. Track Mode 控制

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

## 14. 转向助力控制

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

## 15. 0x3FD 帧 74 个信号完整列表

以下是从 Tesla MCU 固件（libQtCarVAPI.so）提取的 `UI_autopilotControl` 帧全部信号名。
**只有信号名和枚举值，没有 bit 位置。** Bit 位置来自经验逆向（标注来源）。

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

## 16. HW3 vs HW4 差异对照表

### 功能支持差异

| 功能 | HW3 | HW4 | Legacy |
|------|-----|-----|--------|
| FSD 启用 (bit 46) | Yes | Yes | Yes |
| FSD V14 二次锁 (bit 60) | — | Yes | — |
| 紧急车辆检测 (bit 59) | — | Optional | — |
| ECE R79 Nag (bit 19) | 写 0 | 写 0 | 写 0 |
| HW4 Nag 确认 (bit 47) | — | 写 1 | — |
| 速度偏移读取 (mux 0) | Yes | **No** | — |
| 速度偏移注入 (mux 2) | Yes (bit 6-13) | **No** | — |
| 速度档位 (mux 0) | Yes (2 bit, 0-2) | — | Yes (2 bit, 0-2) |
| 速度档位 (mux 2) | — | Yes (3 bit, 0-4) | — |
| ISA 提示音抑制 (0x399) | — | Yes | — |
| Track Mode (0x313) | Yes | — | — |

### CAN ID 使用差异

| CAN ID | HW3 | HW4 | Legacy |
|--------|-----|-----|--------|
| 0x3FD (1021) | 主控帧 | 主控帧 | — |
| 0x3EE (1006) | — | — | 主控帧 |
| 0x399 (921) | — | ISA 抑制 | — |
| 0x3F8 (1016) | 跟车距离 | 跟车距离 | — |
| 0x045 (69) | — | — | 杆位置 |
| 0x313 (787) | Track Mode | — | — |
| 0x370 (880) | Nag Killer | Nag Killer | Nag Killer |

### 速度档位级别差异

| 级别 | 值 | HW3 | HW4 |
|------|---|-----|-----|
| Chill | 0 | Yes | Yes |
| Normal | 1 | Yes | Yes |
| Hurry | 2 | Yes (= Max) | Yes |
| Max | 3 | — | Yes |
| Sloth | 4 | — | Yes |

---

## 17. Model 3/Y vs Model S/X CAN ID 差异

| 信号组 | Model 3/Y | Model S/X |
|--------|-----------|-----------|
| DAS_status (fusedSpeedLimit 等) | 0x399 (921) chassis / 0x39B (923) party | 0x399 (921) |
| DAS_status2 (accSpeedLimit) | 0x389 (905) | 0x389 (905) |
| DAS_control (setSpeed) | 0x2B9 (697) | 0x2B9 (697) |
| UI_driverAssistMapData (mapSpeedLimit) | 0x238 (568) | 0x3C8 (968) |
| UI_gpsVehicleSpeed (userSpeedOffset) | **0x3D9 (985)** | **0x2F8 (760)** |
| UI_powertrainControl (speedLimit) | 0x334 (820) | 0x313 (787) |
| UI_driverAssistRoadSign | 0x218 (536) | 0x238 (568) |
| UI_cruiseControl (speedLimitTick) | 0x213 (531) | — |

---

## 18. DBC 数据来源

| 来源 | 类型 | 覆盖 | URL |
|------|------|------|-----|
| **joshwardell/model3dbc** | DBC | Model 3/Y, bit 位置完整 | https://github.com/joshwardell/model3dbc |
| **commaai/opendbc** | DBC | Model S/X + Model 3/Y (party/vehicle bus) | https://github.com/commaai/opendbc |
| **onyx-m2/onyx-m2-dbc** | DBC | Model 3, 最完整的枚举值 | https://github.com/onyx-m2/onyx-m2-dbc |
| **mikegapinski/tesla-can-explorer** | JSON | 全车型, 577 帧 40484 信号（无 bit 位置） | https://github.com/mikegapinski/tesla-can-explorer |
| **talas9/tesla_can_signals** | JSON | 全车型, 有 bit 位置（但缺 UI/DAS 帧） | https://github.com/talas9/tesla_can_signals |
| **tuncasoftbildik/tesla-can-mod** | C++ | ESP32-C6, 有经验 bit 位置 | https://github.com/tuncasoftbildik/tesla-can-mod |
| **flipper-tesla-fsd** | C | Flipper Zero + ESP32, 经验验证 | 本地参考项目 |
| **sunnypilot/opendbc PR #308** | DBC patch | 限速信号验证 | https://github.com/sunnypilot/opendbc/pull/308 |
| **BYDcar/opendbc-byd** | DBC fork | Model S/X, 含 UI_userSpeedOffset 完整定义 | https://github.com/BYDcar/opendbc-byd |

### 交叉参考方法

```
1. mikegapinski → 查信号名 + 帧地址 + 总线名
2. talas9       → 查 bit 位置 + 宽度（如果信号存在）
3. opendbc      → 查 DBC 定义（较旧但 bit 准确）
4. joshwardell  → 查 Model 3/Y 专用 DBC
5. 逆向代码    → 查经验 bit 位置（tuncasoftbildik, flipper-tesla-fsd）
```

---

## 19. 当前项目实现状态与待办

### 已实现

| 功能 | Handler | 状态 |
|------|---------|------|
| FSD 启用 (bit 46) | HW3, HW4, Legacy | Done |
| FSD V14 二次锁 (bit 60) | HW4 | Done |
| 紧急车辆检测 (bit 59) | HW4 | Done (runtime toggle) |
| ECE R79 Nag (bit 19) | HW3, HW4, Legacy | Done |
| HW4 Nag 确认 (bit 47) | HW4 | Done |
| 速度偏移读取/注入 | HW3 (mux 0 → mux 2) | Done |
| 速度档位 (HW3, mux 0) | HW3, Legacy | Done |
| 速度档位 (HW4, mux 2) | HW4 | Done |
| ISA 提示音抑制 (0x399) | HW4 | Done (runtime toggle) |
| Track Mode (0x313) | HW3 | Done |
| Nag Killer (0x370 echo) | NagHandler | Done (compile-time) |
| 电池预热 (0x082) | app.h 主循环 | Done (runtime toggle) |
| 跟车距离 → 档位映射 | HW3, HW4, Legacy | Done |

### 未实现 / 待办

| 功能 | 原因 | 难度 |
|------|------|------|
| **HW4 速度偏移读取/注入** | mux 0 的偏移 bit 在 HW4 上未验证；mux 2 的 `UI_fsdMaxSpeedOffsetPercentage` bit 位未知 | 需要 CAN 抓包 |
| **限速识别监听** (DAS_fusedSpeedLimit) | CAN 0x399 已在 HW4 filter 中，可以加只读解析 | 低 |
| **用户速度偏移监听** (UI_userSpeedOffset) | CAN 0x3D9 需加入 filter | 低 |
| **ACC 速度上限监听** (DAS_accSpeedLimit) | CAN 0x389 需加入 filter | 低 |
| **DAS 目标速度监听** (DAS_setSpeed) | CAN 0x2B9 需加入 filter | 低 |
| **转向助力控制** (GTW_epasTuneRequest) | CAN 0x101 在 CH 总线上，需不同物理接入 | 中 |

---

## 20. Checksum 计算方法

Tesla 使用统一的 checksum 算法（用于 0x399, 0x370, 0x313 等帧）：

```
checksum = sum(byte[0] .. byte[N-2]) + (CAN_ID & 0xFF) + (CAN_ID >> 8)
byte[N-1] = checksum & 0xFF   // 最后一个字节为 checksum
```

其中 N = DLC（通常为 8），checksum 字节通常是 byte[7]。

代码示例（C++，当前项目 `computeTeslaChecksum` 函数）：
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

各帧的 CAN ID 拆分：

| CAN ID | Hex | Low byte | High byte | 常量和 |
|--------|-----|----------|-----------|--------|
| 921 | 0x399 | 0x99 | 0x03 | 0x9C |
| 880 | 0x370 | 0x70 | 0x03 | 0x73 |
| 787 | 0x313 | 0x13 | 0x03 | 0x16 |

---

*文档结束。最后更新：2026-04-09*
