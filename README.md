## v1.8 版本亮点 / v1.8 Release Highlights

### v1.8.2（最新 / Latest，强烈推荐升级 / Strongly Recommended）
- **修复 智能偏移（Smart Offset）数值被缩小 4 倍**：之前规则设 20% 实际输出仅约 5%，现已修正，规则百分比与 CAN 编码一致
- **修复 智能偏移导致手动模式被「卡死」开启**：关闭智能偏移后手动模式不会再残留；速度偏移现由单一路径处理
- **修复 `/api/smart-offset` `current_pct` 显示陈旧值**：现在实时基于 `fusedSpeedLimit` 查询当前规则

### v1.8.1 / v1.8.1
- **修复 手动驾驶档位被跟随拨杆立即覆盖**：设置手动档位会自动关闭 `profile_mode_auto` 并持久化到 NVS
- **修复 运行时 auto→manual 切换时已保存的手动档位未恢复**：切换后会立即从 NVS 还原

### v1.8.0 / v1.8.0
- **修复 `handlers.h` 结构损坏**：恢复 `#pragma once`、includes、`CarManagerBase`、`LegacyHandler`
- **修复 智能偏移从未生效**：HW3/HW4 现在读取 NVS 中的智能偏移规则，并通过帧 921 解码 `fusedSpeedLimit`
- **新增 HW3 对帧 921 的解码**，用于智能偏移
- **速度偏移优先级明确化**：手动模式 > 智能偏移 > CAN 原始值，由 `computeSpeedOffset()` 统一实现

### 升级建议 / Upgrade Notes
- 推荐所有用户升级至 1.8.2，修复了 1.8.0/1.8.1 中的关键缺陷
- 升级后如果「手动偏移模式」开关残留为开启状态（旧版副作用），请手动关闭一次即可

### 🚨 请勿将你的 Tesla 升级到 `2026.8.6`、`2026.2.9.x` 或 `2026.2.300`，以保留 FSD 功能 🚨

<br>
<hr>

# Tesla Open CAN Mod

> 本项目是 [Tesla-OPEN-CAN-MOD](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod) 的一个分支。感谢原作者的开源贡献。
> This README is bilingual: Simplified Chinese first, with English labels and section titles for quick navigation.

这是一个面向 Tesla 车辆的开源通用 CAN 总线修改工具。项目最初从启用 FSD 出发，但目标是暴露并控制所有可通过 CAN 访问的功能，包括速度控制、nag 抑制、电池预热、ISA 提示音静音、紧急车辆感知等。

**当前版本：** 1.8.2 · **平台：** ESP32 / ESP32-S3 · **接口：** TWAI（CAN）· **语言：** C++（PlatformIO + Arduino 框架）

## 免责声明 / Disclaimer

> [!WARNING]
> **FSD 是付费功能，必须已正确购买或订阅。** 任何试图绕过购买或订阅要求的行为，都可能导致 Tesla 服务永久封禁。

> [!WARNING]
> **修改 CAN 总线报文可能导致危险行为，甚至永久损坏车辆。** CAN 总线控制从制动、转向到安全气囊等几乎所有系统，错误报文可能带来严重后果。如果你不能完全理解自己在做什么，**请不要把它安装到车上**。

本项目仅用于测试和教育目的，且只适用于**私人场地**。作者对因使用本软件而造成的车辆损坏、人身伤害或法律后果概不负责。本项目可能使车辆保修失效，并且**可能不符合你所在地区的道路安全法规**。

如果你的使用超出私人测试范围，你需要自行遵守所有适用的当地法律法规。行驶时务必双手扶方向盘并保持注意力集中。

---

# English Version

## v1.8 Highlights

### v1.8.2 (latest, strongly recommended)
- **Fixed Smart Offset being scaled down by 4x**: rules such as 20% were previously outputting only about 5%; the percentage now matches the CAN encoding again
- **Fixed Smart Offset getting manual mode stuck on**: disabling smart mode no longer leaves manual mode latched; speed offset now has a single control path
- **Fixed stale `current_pct` in `/api/smart-offset`**: the UI now queries the live rule set using `fusedSpeedLimit`

### v1.8.1
- **Fixed manual profile being overwritten by the follow-distance stalk**: setting a manual profile now disables `profile_mode_auto` and persists it in NVS
- **Fixed auto→manual switching not restoring the saved manual profile**: switching modes now restores the value from NVS

### v1.8.0
- **Fixed the broken `handlers.h` structure**: restored `#pragma once`, includes, `CarManagerBase`, and `LegacyHandler`
- **Fixed Smart Offset never taking effect**: HW3/HW4 now load the persisted smart-offset rules and decode `fusedSpeedLimit` from frame 921
- **Added frame 921 decoding on HW3** for Smart Offset
- **Clarified the speed-offset priority**: manual mode > smart offset > raw CAN, all through `computeSpeedOffset()`

### Upgrade notes
- Upgrade all users to 1.8.2; it fixes the critical issues from 1.8.0 and 1.8.1
- If the old "manual offset mode" toggle is still latched on after upgrading, turn it off once manually

### 🚨 Do not update your Tesla to `2026.8.6`, `2026.2.9.x`, or `2026.2.300` if you want to preserve FSD functionality 🚨

<br>
<hr>

# Tesla Open CAN Mod

> This project is a fork of [Tesla-OPEN-CAN-MOD](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod). Credit goes to the original author for the open-source work.

This is an open-source, general-purpose CAN bus modification tool for Tesla vehicles. The project started as a way to enable FSD, but the goal is broader: expose and control everything reachable via CAN, including speed control, nag suppression, battery preconditioning, ISA chime muting, emergency vehicle awareness, and more.

**Current Version:** 1.8.2 · **Platform:** ESP32 / ESP32-S3 · **Interface:** TWAI (CAN) · **Language:** C++ (PlatformIO + Arduino framework)

## Disclaimer

> [!WARNING]
> **FSD is a paid feature and must be properly purchased or subscribed to.** Any attempt to bypass the purchase or subscription requirement may result in permanent Tesla service bans.

> [!WARNING]
> **Modifying CAN bus traffic can lead to dangerous behavior or permanent vehicle damage.** CAN control touches almost every subsystem, from braking and steering to airbags. Invalid frames can have severe consequences. If you do not fully understand what you are doing, **do not install this on a car**.

This project is for testing and educational purposes only, and only for use on **private property**. The author is not responsible for vehicle damage, personal injury, or legal consequences caused by using this software. It may void your vehicle warranty and may **not comply with road-safety laws in your region**.

If you use it outside private testing, you are responsible for complying with all applicable local laws. Always keep both hands on the wheel and stay attentive while driving.

## 功能 / Features

### CAN 总线改动 / CAN Bus Modifications

- **FSD 激活 / FSD Activation** — 通过拦截并修改 Autopilot 控制帧（0x3FD / 0x3EE）在 CAN 层启用 FSD
- **Nag 抑制 / Nag Suppression** — 清除方向盘施力检测的 ECE R79 nag bit，抑制周期性的“请对方向盘施加压力”提示
- **Autosteer Nag Killer / Autosteer Nag Killer** — 通过在 CAN 4 上回显修改后的 EPAS3P_sysStatus 帧（0x370），伪造 1.80 Nm 扭矩并递增计数器，从而抑制 Autopilot 的“请握住方向盘”警告
- **增强版自动辅助驾驶 / Enhanced Autopilot** — 启用 Enhanced Autopilot 功能，包括召唤（HW3/HW4，可选）
- **速度档位 / Speed Profiles** — 5 个驾驶激进程度档位（Chill / Normal / Hurry / Max / Sloth）；自动模式会根据跟车距离拨杆映射，或通过 Web 界面手动设置
- **手动速度偏移 / Manual Speed Offset** — 在 Autopilot 目标速度之上叠加 0–50% 的速度偏移（HW3/HW4）
- **智能速度偏移 / Smart Speed Offset** — 最多 8 条基于速度的规则，会根据当前融合限速自动调整偏移。默认规则：≤40→50%、50→30%、60→20%、80+→10%
- **ISA 限速提示音抑制 / ISA Speed Chime Suppression** — 在保留视觉提示的同时，静音 Intelligent Speed Assistance 的提示音（HW4，可选）
- **紧急车辆检测 / Emergency Vehicle Detection** — 启用 FSD v14 上的紧急车辆接近检测（HW4，可选）
- **电池预热 / Battery Preheat** — 通过每 500 ms 注入 UI_tripPlanning 帧（0x082）触发 BMS 电池预调节。可配置按温度（-5–25°C）和最长时间（10–60 分钟）自动停止
- **中国模式 / China Mode** — 绕过交通灯/停车标志 UI 选择检查，适用于中国地区固件

### Web 界面 / Web Interface

内置 WiFi 热点（AP 模式，SSID 为 `TeslaCAN`）提供功能完整的单页 Web 仪表盘。可选 STA 模式可连接家庭/车库 WiFi 网络。

**仪表盘标签页 / Dashboard Tab**
- 实时信号监控：油门、限速、方向盘施力等级、跟车距离、档位
- 电池状态：SoC%、电压、电流、最低/最高温度
- 实时速度/SoC/功率图表（Canvas，120 点环形缓冲区）
- 速度档位选择器和偏移控制

**设置标签页 / Settings Tab**
- 运行时硬件模式切换（Legacy / HW3 / HW4），支持 NVS 持久化
- 功能开关：BYPASS TLSSC、ISA Chime、Emergency Vehicle、Enhanced AP、Nag Killer、China Mode
- 预热控制和状态面板（运行时长、电池温度、自动停止配置）

**系统标签页 / System Tab**
- 系统信息：芯片温度、处理延迟（平均/最大）、可用堆内存/PSRAM
- WiFi SSID/密码配置
- OTA 固件上传（带进度条和 MD5 校验）
- 驾驶数据记录：带 CSV 导出的环形缓冲区（速度、加速度、转向、电池、温度）
- 实时日志查看器（256 条目环形缓冲区）

**界面特性 / UI Features**
- 根据车辆车灯状态自动切换深色/浅色主题（CAN 0x185）
- 双语 i18n：简体中文 / 英文，可在运行时切换
- 支持 captive portal（iOS / Android 自动跳转）

### 安全与可靠性 / Safety & Reliability

- **OTA 保护** — 检测到车辆 OTA 更新时，会自动暂停所有 CAN 修改；完成后自动恢复
- **NVS 损坏恢复** — 启动时检测 NVS Flash 损坏，自动擦除并使用默认值重新初始化
- **预热安全保护** — 按温度阈值和最长时间自动停止；重启后不持久化（需要手动重新开启）
- **速度偏移警告** — 首次启用非零速度偏移时弹出安全确认
- **速率限制** — 所有 POST 端点都强制 500 ms 最小间隔（超限返回 429）
- **单次发送** — 预热注入使用 TWAI_MSG_FLAG_SS，避免 CAN 重试风暴

### 性能 / Performance

- **O(1) CAN 过滤** — 32 元素位掩码数组覆盖 0–1023 号 ID；每帧无需线性搜索
- **O(1) 信号查找** — 1024 元素直接映射表用于 CAN Live 信号更新
- **零分配 JSON** — canLiveHandler 和 statusHandler 使用 snprintf 组装静态缓冲区（无需每次请求分配 cJSON 树）
- **单次解码** — OTA 检查、智能偏移和预热共享同一次 `decodeSignals()` 调用
- **Canvas 节流** — 图表和 CAN 表格使用 `requestAnimationFrame` 并检查可见性；标签页隐藏时跳过重绘
- **缓存 DOM 查询** — 重复的 `querySelectorAll` 结果会在首次访问后缓存

## CAN 信号参考 / CAN Signal Reference

下表列出本固件使用的所有 CAN 信号。**修改** 信号会被拦截并以变更后的位重新发送。**注入** 信号由固件生成。**仅监控** 信号只用于 Web 仪表盘的被动解码。

### 修改与注入信号 / Modified & Injected Signals

| CAN ID | 名称 / Name | 方向 / Direction | 处理器 / Handler | 功能 / Function |
|--------|------|------|--------|------|
| `0x045` (69) | STW_ACTN_RQ | 读取 | Legacy | 读取跟车距离拨杆位置，用于映射速度档位 |
| `0x082` (130) | UI_tripPlanning | **注入** | Preheat | 每 500 ms 注入 `byte[0]=0x05`，触发 BMS 电池预调节 |
| `0x313` (787) | TrackModeRequest | **修改** | HW3 | 设置赛道模式请求位（`byte[0] bit 0-1 = 0x01`），并重算校验和 |
| `0x370` (880) | EPAS3P_sysStatus | **注入** | NagKiller | 当 `handsOnLevel=0` 时，回显伪造帧，设置 `torsionBarTorque=1.80 Nm`、`handsOnLevel=1`、计数器 +1，并重算校验和 |
| `0x399` (921) | DAS_status | **修改** | HW4 | ISA 限速提示音抑制：设置 `byte[1]` 的 0x20 位并重算校验和 |
| `0x3EE` (1006) | UI_autopilotControl | **修改** | Legacy | Mux 0：启用 FSD（`bit 46`），设置速度档位。Mux 1：清除 ECE R79 nag（`bit 19`） |
| `0x3F8` (1016) | UI_driverAssistControl | 读取 | HW3 / HW4 | 读取跟车距离设置（`byte[5] bits 5-7`），用于自动映射速度档位 |
| `0x3FD` (1021) | UI_autopilotControl | **修改** | HW3 / HW4 | Mux 0：启用 FSD（`bit 46`），读写速度偏移，紧急车辆检测（`bit 59`，HW4）。Mux 1：清除 ECE R79 nag（`bit 19`），启用增强版 AP（`bit 47`，HW4）。Mux 2：写入速度偏移和速度档位 |

### 仅监控信号（Web 仪表盘） / Monitor-Only Signals (Web Dashboard)

这些信号会被动地从 CAN 总线解码，用于实时仪表盘显示。它们**不会被修改或重新发送**。

These signals are passively decoded from the CAN bus for the live dashboard. They are **never modified or re-transmitted**.

| CAN ID | 名称 / Name | 解码字段 / Decoded Fields | 单位 / Unit |
|--------|------|----------|------|
| `0x118` (280) | DI_systemStatus | 档位、油门踏板位置 | —, % |
| `0x132` (306) | BMS_hvBusStatus | 电池包电压、电流 | V, A |
| `0x185` (389) | Lighting | 灯光状态、原始字节 3-4 | — |
| `0x238` (568) | UI_driverAssistMapData | 地图限速 | kph |
| `0x257` (599) | DI_speed | 车速、界面显示速度、速度单位 | kph, kph/mph |
| `0x292` (658) | BMS_socStatus | 电池荷电状态 | % |
| `0x2B9` (697) | DAS_control | DAS 目标速度、ACC 状态 | kph, 枚举 |
| `0x312` (786) | BMS_thermalStatus | 电池最低/最高温度 | °C |
| `0x318` (792) | GTW_carState | OTA 更新进行中标志 | bool |
| `0x334` (820) | UI_powertrainControl | 整车速度限制 | kph |
| `0x370` (880) | EPAS3P_sysStatus | 方向盘施力等级、扭矩、计数器 | 枚举, Nm, — |
| `0x389` (905) | DAS_status2 | ACC 速度上限 | kph |
| `0x399` (921) | DAS_status | 融合限速、视觉限速、超速警告抑制 | kph, kph, bool |
| `0x3D9` (985) | UI_gpsVehicleSpeed | GPS 速度、用户速度偏移、MPP 限速 | kph, kph, kph |
| `0x3F8` (1016) | UI_driverAssistControl | 跟车距离设置 | 1-7 |
| `0x3FD` (1021) | UI_autopilotControl | FSD UI 选择、FSD 启用、HW4 锁、速度档位、速度偏移、ECE R79 nag | — |

> **注意：** `0x145`（ESP_status brake torque）和 `0x261`（motor torque）在解码器中已定义，但目前已禁用，因为它们在 X179 CAN 总线上的字节位置尚未验证。

## 工作原理 / How It Works

固件运行在支持原生 TWAI（CAN）外设的 ESP32 和 ESP32-S3 板子上。它位于车辆 CAN 总线上，拦截相关报文，修改必要位，并把修改后的报文重新发送——全部实时完成。

一个基于位掩码的 O(1) 过滤器决定当前处理器需要哪些 CAN ID。非匹配报文会原样放行。硬件变体（Legacy / HW3 / HW4）可以在编译时指定，也可以在运行时切换（仅限支持 `RUNTIME_HW_SWITCH` 的 ESP32-S3 板子）。WiFi Web 界面提供运行时控制、实时监控和 OTA 固件更新。

### 架构 / Architecture

```
车辆 CAN 总线
      │
  ┌───┴───┐
  │ TWAI  │  ESP32/ESP32-S3
  │ 驱动   │
  └───┬───┘
      │
  ┌───┴──────────┐
  │  主循环      │  ~1 ms 周期
  │  ┌────────┐  │
  │  │ 过滤器 │──┤── O(1) 位掩码检查
  │  └───┬────┘  │
  │  ┌───┴────┐  │
  │  │ 处理器 │──┤── Legacy / HW3 / HW4
  │  └───┬────┘  │
  │  ┌───┴────┐  │
  │  │ 解码   │──┤── 信号监控（每轮 1 次）
  │  └────────┘  │
  └──────────────┘
      │
  ┌───┴──────────┐
  │  Web 服务器  │  HTTP :80
  │  29 个端点    │
  │  WiFi AP+STA  │
  └──────────────┘
```

## 先决条件 / Prerequisites

**使用 FSD 相关功能需要有效的 FSD 套餐** —— 无论是购买还是订阅。这个板子只是在 CAN 层启用 FSD 功能，但车辆本身仍然需要 Tesla 的有效 FSD 权益。

Autosteer Nag Killer、ISA 限速提示音抑制、电池预热和 Web 界面等功能可以独立使用，不需要 FSD。

如果你所在地区无法使用 FSD 订阅，可以通过海外 Tesla 账号作为变通方案。

## 支持的开发板 / Supported Boards

| 开发板 / Board | CAN 接口 / CAN Interface | PSRAM | 状态 / Status |
|--------|----------|-------|------|
| [Waveshare ESP32-S3 RS485/CAN](https://www.waveshare.com/esp32-s3-rs485-can.htm) | SIT1050T + ESP32-S3 TWAI | 8 MB | 推荐 |
| ESP32 + CAN 收发器（例如 ESP32-DevKitC + SN65HVD230） | 原生 TWAI 外设 | — | 已测试 |
| [Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G + ESP32 TWAI | — | 已测试 |
| M5Stack AtomS3 + CAN Base | CA-IS3050G + ESP32-S3 TWAI | — | 已测试 |
| LilyGo T-CAN485 | ESP32 TWAI + SN65HVD230 | — | 已测试 |

## 构建与刷写 / Build & Flash

使用 PlatformIO 进行构建和刷写。根据你的开发板选择环境，并在 `platformio.ini` 的 build flags 中配置车辆变体。

```bash
# 为 Waveshare ESP32-S3 构建
pio run -e waveshare_esp32s3_rs485_can

# 为通用 ESP32 构建
pio run -e esp32_twai

# 通过 USB 刷写
pio run -e waveshare_esp32s3_rs485_can -t upload

# 运行单元测试（120 个原生测试）
pio test -e native
```

### 构建参数 / Build Flags

| 参数 / Flag | 默认值 / Default | 说明 / Description |
|------|--------|------|
| `-DHW4` | 开启（esp32_twai） | 目标 HW4 车辆处理器 |
| `-DBYPASS_TLSSC_REQUIREMENT` | 关闭 | 绕过 Traffic Light & Stop Sign Control UI 开关 |
| `-DISA_SPEED_CHIME_SUPPRESS` | 开启 | 启用 ISA 提示音静音（HW4） |
| `-DEMERGENCY_VEHICLE_DETECTION` | 开启 | 启用紧急车辆感知（HW4） |
| `-DENHANCED_AUTOPILOT` | 开启 | 启用增强版 Autopilot |
| `-DNAG_KILLER` | 关闭 | 编译 Autosteer Nag Killer（需要接入 X179 总线） |
| `-DRUNTIME_HW_SWITCH` | 开启（S3） | 允许运行时切换 Legacy/HW3/HW4 模式 |

## API 端点 / API Endpoints

Web 服务器暴露 29 个 HTTP 端点。所有功能开关都接受 `{"enabled": bool}` 并返回 `{"ok": true}`。所有 POST 端点都限制为 500 ms 一次。

The web server exposes 29 HTTP endpoints. All feature toggles accept `{"enabled": bool}` and return `{"ok": true}`. POST endpoints are rate-limited to 500 ms.

| 方法 / Method | 路径 / Path | 说明 / Description |
|------|------|------|
| GET | `/` | Web 界面（单页 HTML） |
| GET | `/api/status` | 完整系统状态 JSON（功能、CAN 统计、日志） |
| GET | `/api/can-live` | 实时 CAN 信号 |
| GET | `/api/smart-offset` | 智能偏移规则和当前百分比 |
| GET | `/api/preheat/status` | 预热状态（是否激活、温度、运行时长） |
| GET | `/api/can-log/status` | CAN 监控缓冲区状态 |
| GET | `/api/can-log/dump` | 二进制 CAN 日志下载（分块传输） |
| GET | `/api/drive-data/status` | 驾驶数据记录状态 |
| GET | `/api/drive-data/csv` | 驾驶数据 CSV 下载 |
| POST | `/api/bypass-tlssc` | 切换 Bypass TLSSC |
| POST | `/api/isa-speed-chime-suppress` | 切换 ISA 提示音抑制 |
| POST | `/api/emergency-vehicle-detection` | 切换紧急车辆检测 |
| POST | `/api/enhanced-autopilot` | 切换增强版 Autopilot |
| POST | `/api/nag-killer` | 切换 Autosteer Nag Killer |
| POST | `/api/china-mode` | 切换中国模式 |
| POST | `/api/hw-mode` | 切换硬件模式（需要重启） |
| POST | `/api/profile-mode-auto` | 切换自动速度档位 |
| POST | `/api/speed-profile` | 设置手动速度档位（0–4） |
| POST | `/api/speed-offset` | 设置手动速度偏移 |
| POST | `/api/smart-offset` | 配置智能偏移规则 |
| POST | `/api/preheat` | 切换电池预热 |
| POST | `/api/preheat/config` | 设置预热自动停止阈值 |
| POST | `/api/wifi` | 保存 WiFi STA 凭据（触发重启） |
| POST | `/api/ota` | 上传固件二进制（支持通过请求头传入 MD5） |
| POST | `/api/enable-print` | 切换串口日志输出 |
| POST | `/api/drive-data/toggle` | 开始/停止驾驶数据记录 |
| POST | `/api/drive-data/clear` | 清空驾驶数据缓冲区 |
| POST | `/api/can-log/start` | 开始 CAN 监控日志 |
| POST | `/api/can-log/stop` | 停止 CAN 监控日志 |
| POST | `/api/can-log/clear` | 清空 CAN 监控缓冲区 |

## NVS 持久化 / NVS Persistence

所有功能开关、速度偏移设置、智能偏移规则、WiFi 凭据和硬件模式都会持久化到 ESP32 NVS Flash。设置会在重启和断电后保留。固件在启动时会检测 NVS 损坏，并通过擦除并重新初始化恢复到默认值。

**例外：** 电池预热故意不持久化 —— 为了安全，重启后必须手动重新开启。

## 版本管理 / Versioning

- 项目版本记录在 [`VERSION`](VERSION) 中，采用语义化版本管理。
- 发布说明记录在 [`CHANGELOG.md`](CHANGELOG.md) 中。

## 第三方库 / Third-Party Libraries

本项目依赖以下开源库。完整许可证文本见 [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)。

This project depends on the following open-source libraries. Full license texts are in [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES).

| 库 / Library | 许可证 / License | 版权 / Copyright |
|----|--------|------|
| [espressif/esp-idf](https://github.com/espressif/esp-idf)（TWAI 驱动） | Apache 2.0 | (c) 2015-2025 Espressif Systems (Shanghai) CO LTD |

## 许可证 / License

本项目采用 **GNU General Public License v3.0** 许可，详见 [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html)。