### 🚨 请勿将特斯拉更新至 `2026.8.6`、`2026.2.9.x` 及 `2026.2.300`，以保留 FSD 功能 🚨

<br>
<hr>

# Tesla Open Can Mod

> 本项目基于 [Tesla-OPEN-CAN-MOD](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod) 大佬的原始项目进行二次开发，感谢原作者的开源贡献。

开源 Tesla CAN 总线通用改装工具。以 FSD 激活为起点，目标是通过 CAN 总线暴露并控制一切可访问的功能。

## 免责声明

> [!WARNING]
> **FSD 是付费高级功能，必须正规购买或订阅。** 任何试图绕过购买或订阅要求的行为都将导致被 Tesla 服务永久封禁。

> [!WARNING]
> **修改 CAN 总线消息可能导致危险行为或永久损坏车辆。** CAN 总线控制着从制动、转向到安全气囊的一切——一条错误的消息可能造成严重后果。如果你不完全了解自己在做什么，**请勿将此设备安装在你的车上**。

本项目仅供测试和教育用途，仅限在**私人场地**使用。作者不对因使用本软件造成的任何车辆损坏、人身伤害或法律后果承担责任。本项目可能导致车辆保修失效，并且**可能不符合你所在地区的道路安全法规**。

在私人测试之外的任何使用，你有责任遵守所有适用的当地法律法规。驾驶时请始终将手放在方向盘上并保持注意力集中。

## 功能

- **FSD 激活** — 通过拦截和修改 Autopilot 控制帧，在 CAN 总线层面启用完全自动驾驶
- **Nag 抑制** — 清除 ECE R79 规定的握方向盘提醒位，抑制周期性的"请对方向盘施加压力"警告
- **Autosteer Nag Killer** — 通过在 CAN 总线 4 上发送修改后的 EPAS 转向扭矩帧来抑制 Autopilot"手放方向盘"提示
- **Actually Smart Summon (ASS)** — 移除智能召唤的欧盟法规限制（HW3/HW4）
- **速度档位** — 将跟车距离拨杆设置映射为 FSD 激进程度档位（保守 / 默认 / 适中 / 激进 / 树懒）
- **速度偏移** — 在 Autopilot 设定速度之上应用可调速度偏移。支持手动偏移和基于速度规则的智能偏移模式（HW3/HW4）
- **ISA 速度提示音抑制** — 静音智能速度辅助的声音提示，同时保留视觉指示器（HW4，可选）
- **紧急车辆检测** — 在 FSD v14 上启用接近紧急车辆检测（HW4，可选）
- **电池预热管理** — 通过注入导航行程帧触发 BMS 电池预加热（测试功能）
- **Web 管理界面** — ESP32 板载 WiFi 热点，提供实时监控、运行时功能开关和 OTA 固件更新

## 工作原理

固件运行在 ESP32 和 ESP32-S3 板卡上，使用原生 TWAI（CAN）外设。它挂载在车辆 CAN 总线上，拦截相关帧，修改必要的位，然后重新发送修改后的帧——全部实时完成。

硬件类型（Legacy / HW3 / HW4）和可选功能通过 `platformio.ini` 构建标志配置。WiFi Web 界面提供运行时控制、实时监控和 OTA 固件更新。

## 前提条件

**使用 FSD 相关功能需要有效的 FSD 套餐** — 购买或订阅均可。本设备在 CAN 总线层面启用 FSD 功能，但车辆仍需要 Tesla 的有效 FSD 授权。

Autosteer Nag Killer、ISA 速度提示音抑制和 Web 界面等功能独立运行，不需要 FSD。

如果你所在地区无法订阅 FSD，可以通过外区 Tesla 账户解决。

## 支持的板卡

| 板卡                                                                     | CAN 接口                    | 状态       |
|-------------------------------------------------------------------------|----------------------------|------------|
| [Waveshare ESP32-S3 RS485/CAN](https://www.waveshare.com/esp32-s3-rs485-can.htm) | SIT1050T + ESP32-S3 TWAI   | 推荐       |
| ESP32 + CAN 收发器（如 ESP32-DevKitC + SN65HVD230）                      | 原生 TWAI 外设              | 已测试     |
| [Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G + ESP32 TWAI    | 已测试     |
| M5Stack AtomS3 + CAN Base                                               | CA-IS3050G + ESP32-S3 TWAI | 已测试     |
| LilyGo T-CAN485                                                         | ESP32 TWAI + SN65HVD230    | 已测试     |

## 安装

使用 PlatformIO 编译和烧录。选择你的板卡环境并在 `platformio.ini` 构建标志中配置车辆类型。

## 版本管理

- 项目版本在 [`VERSION`](VERSION) 中使用语义化版本号跟踪。
- 发行说明在 [`CHANGELOG.md`](CHANGELOG.md) 中记录。

## 第三方库

本项目依赖以下开源库。完整许可证文本见 [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES)。

| 库 | 许可证 | 版权 |
|---------|---------|-----------|
| [espressif/esp-idf](https://github.com/espressif/esp-idf)（TWAI 驱动） | Apache 2.0 | (c) 2015-2025 Espressif Systems (Shanghai) CO LTD |

## 许可证

本项目采用 **GNU 通用公共许可证 v3.0** 授权 — 详见 [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html)。
