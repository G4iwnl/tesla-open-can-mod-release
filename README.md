### 🚨 DO NOT UPDATE YOUR TESLA TO ```2026.8.6``` and ```2026.2.9.x``` TO KEEP FSD FEATURES 🚨

<br>
<hr>

# Tesla Open Can Mod

> 本项目基于 [Tesla-OPEN-CAN-MOD](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod) 大佬的原始项目进行二次开发，感谢原作者的开源贡献。

[Website](https://teslaopencanmod.org) | [Documentation](https://teslaopencanmod.org/docs/intro) | [Community Discord](https://discord.gg/ZTQKAUTd2F)

---

开源 Tesla CAN 总线通用改装工具。以 FSD 激活为起点，目标是通过 CAN 总线暴露并控制一切可访问的功能。

## Disclaimer

> [!WARNING]
> **FSD is a premium feature and must be properly purchased or subscribed to.** Any attempt to bypass the purchase or subscription requirement will result in a permanent ban from Tesla services.

> [!WARNING]
> **Modifying CAN bus messages can cause dangerous behavior or permanently damage your vehicle.** The CAN bus controls everything from braking and steering to airbags — a malformed message can have serious consequences. If you don't fully understand what you're doing, **do not install this on your car**.

This project is for testing and educational purposes only and for use on **private property**. The authors accept no responsibility for any damage to your vehicle, injury, or legal consequences resulting from the use of this software. This project may void your vehicle warranty and **may not comply with road safety regulations in your jurisdiction**.

For any use beyond private testing, you are responsible for complying with all applicable local laws and regulations. Always keep your hands on the wheel and stay attentive while driving.

## Features

- **FSD Activation** — Enables Full Self-Driving at the CAN bus level by intercepting and modifying Autopilot control frames
- **Nag Suppression** — Clears the hands-on-wheel ECE R79 nag bit, suppressing the periodic "apply pressure to the steering wheel" warning
- **Autosteer Nag Killer** — Suppresses the Autopilot "hands on wheel" alert by echoing modified EPAS steering torque frames on CAN bus 4
- **Actually Smart Summon (ASS)** — Removes EU regulatory restrictions on Smart Summon (HW3/HW4)
- **Speed Profiles** — Maps the follow-distance stalk setting to FSD aggressiveness profiles (Chill / Normal / Hurry / Max / Sloth)
- **Speed Offset** — Adjustable speed offset applied on top of the Autopilot set speed. Supports manual offset and smart offset mode with speed-based rules (HW3/HW4)
- **ISA Speed Chime Suppression** — Mutes the Intelligent Speed Assistance audible chime while keeping the visual indicator active (HW4, optional)
- **Emergency Vehicle Detection** — Enables approaching emergency vehicle detection on FSD v14 (HW4, optional)
- **Web Interface** — WiFi hotspot on ESP32 boards for real-time monitoring, runtime feature toggles, and over-the-air firmware updates

Full feature documentation: [teslaopencanmod.org/docs/](https://teslaopencanmod.org/docs/intro)

## What It Does

The firmware runs on ESP32 and ESP32-S3 boards with the native TWAI (CAN) peripheral. It sits on the vehicle CAN bus, intercepts relevant frames, modifies the necessary bits, and re-transmits the modified frames — all in real time.

The hardware variant (Legacy / HW3 / HW4) and optional features are configured via `platformio.ini` build flags. A WiFi web interface provides runtime control, real-time monitoring, and over-the-air firmware updates.

## Prerequisites

**An active FSD package is required to use FSD-related features** — either purchased or subscribed. This board enables the FSD functionality on the CAN bus level, but the vehicle still needs a valid FSD entitlement from Tesla.

Features like the Autosteer Nag Killer, ISA Speed Chime Suppression, and the Web Interface work independently and do not require FSD.

If FSD subscriptions are not available in your region, there is a workaround using a foreign Tesla account. See [teslaopencanmod.org/docs/getting-started/fsd-subscription](https://teslaopencanmod.org/docs/getting-started/fsd-subscription) for details.

## Supported Boards

| Board                                                                   | CAN Interface              | Status      |
|-------------------------------------------------------------------------|----------------------------|-------------|
| [Waveshare ESP32-S3 RS485/CAN](https://www.waveshare.com/esp32-s3-rs485-can.htm) | SIT1050T over ESP32-S3 TWAI | Recommended |
| ESP32 with CAN transceiver (e.g. ESP32-DevKitC + SN65HVD230)            | Native TWAI peripheral     | Tested      |
| [Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G over ESP32 TWAI | Tested      |
| M5Stack AtomS3 on CAN Base                                              | CA-IS3050G over ESP32-S3 TWAI | Tested   |
| LilyGo T-CAN485                                                         | ESP32 TWAI + SN65HVD230   | Tested      |

## Installation

Use PlatformIO to build and flash. Select your board environment and configure the vehicle variant in `platformio.ini` build flags.

Full installation guide: [teslaopencanmod.org/docs/getting-started/firmware-flash](https://teslaopencanmod.org/docs/getting-started/firmware-flash)

## Versioning

- The project version is tracked in [`VERSION`](VERSION) using Semantic Versioning.
- Release notes are tracked in [`CHANGELOG.md`](CHANGELOG.md).
- Ongoing work should be added to the `Unreleased` section before merge.

## Third-Party Libraries

This project depends on the following open-source libraries. Their full license texts are in [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES).

| Library | License | Copyright |
|---------|---------|-----------|
| [espressif/esp-idf](https://github.com/espressif/esp-idf) (TWAI driver) | Apache 2.0 | (c) 2015-2025 Espressif Systems (Shanghai) CO LTD |

## License

This project is licensed under the **GNU General Public License v3.0** — see the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html) for details.
