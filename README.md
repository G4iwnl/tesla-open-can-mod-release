### 🚨 DO NOT UPDATE YOUR TESLA TO ```2026.8.6``` and ```2026.2.9.x``` TO KEEP FSD FEATURES 🚨

<br>
<hr>

# Tesla Open Can Mod

[Website](https://teslaopencanmod.org) | [Documentation](https://teslaopencanmod.org/docs/intro) | [Community Discord](https://discord.gg/ZTQKAUTd2F)

An open-source general-purpose CAN bus modification tool for Tesla vehicles. While FSD enablement was the starting point, the goal is to expose and control everything accessible via CAN — as a fully open project.

Some sellers charge up to 500 € for a solution like this. The hardware costs around 20 €, and even with labor factored in, a fair price is no more than 50 €. This project exists so nobody has to overpay.

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
- **ISA Speed Chime Suppression** — Mutes the Intelligent Speed Assistance audible chime while keeping the visual indicator active (HW4, optional)
- **Emergency Vehicle Detection** — Enables approaching emergency vehicle detection on FSD v14 (HW4, optional)
- **Web Interface** — WiFi hotspot on ESP32 boards for real-time monitoring, runtime feature toggles, and over-the-air firmware updates

Full feature documentation: [teslaopencanmod.org/docs/](https://teslaopencanmod.org/docs/intro)

## What It Does

The firmware runs on multiple supported boards — Adafruit Feather RP2040 CAN, Feather M4 CAN Express, ESP32-based boards, and M5Stack Atomic CAN Base. It sits on the vehicle CAN bus, intercepts relevant frames, modifies the necessary bits, and re-transmits the modified frames — all in real time.

Features are selected at compile time via `sketch_config.h` and vary by hardware variant (Legacy / HW3 / HW4). ESP32 boards additionally expose a WiFi web interface for runtime control and OTA firmware updates.

## Prerequisites

**An active FSD package is required to use FSD-related features** — either purchased or subscribed. This board enables the FSD functionality on the CAN bus level, but the vehicle still needs a valid FSD entitlement from Tesla.

Features like the Autosteer Nag Killer, ISA Speed Chime Suppression, and the Web Interface work independently and do not require FSD.

If FSD subscriptions are not available in your region, there is a workaround using a foreign Tesla account. See [teslaopencanmod.org/docs/getting-started/fsd-subscription](https://teslaopencanmod.org/docs/getting-started/fsd-subscription) for details.

## Supported Boards

| Board                                                                   | CAN Interface              | Library                      | Status                             | Case STL |
|-------------------------------------------------------------------------|----------------------------|------------------------------|------------------------------------|----------|
| Adafruit Feather RP2040 CAN                                             | MCP2515 over SPI           | `mcp2515.h` (autowp)         | Tested                             | [Printables](https://www.printables.com/model/1662242-adafruit-rp2040-can-bus-feather-case-5724) |
| Adafruit Feather M4 CAN Express (ATSAME51)                              | Native MCAN peripheral     | `Adafruit_CAN` (`CANSAME5x`) | Tested                             |          |
| ESP32 with CAN transceiver (e.g. ESP32-DevKitC + SN65HVD230)            | Native TWAI peripheral     | ESP-IDF `driver/twai.h`      | Tested                             |          |
| [Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G over ESP32 TWAI | ESP32 TWAI                   | Tested                             |          |
| Adafruit ESP32 Feather V2 (5400) + Adafruit CAN Bus Featherwing (5709)  | MCP2515 over SPI           | `mcp2515.h` (autowp)         | Tested                             |          |

## Installation

Both Arduino IDE and PlatformIO are supported. Select your board and vehicle variant in `sketch_config.h`, then flash the firmware to your board.

Full installation guide: [teslaopencanmod.org/docs/getting-started/firmware-flash](https://teslaopencanmod.org/docs/getting-started/firmware-flash)

## Versioning

- The project version is tracked in [`VERSION`](VERSION) using Semantic Versioning.
- Release notes are tracked in [`CHANGELOG.md`](CHANGELOG.md).
- Ongoing work should be added to the `Unreleased` section before merge.

## Third-Party Libraries

This project depends on the following open-source libraries. Their full license texts are in [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES).

| Library | License | Copyright |
|---------|---------|-----------|
| [autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515) | MIT | (c) 2013 Seeed Technology Inc., (c) 2016 Dmitry |
| [adafruit/Adafruit_CAN](https://github.com/adafruit/Adafruit_CAN) | MIT | (c) 2017 Sandeep Mistry |
| [espressif/esp-idf](https://github.com/espressif/esp-idf) (TWAI driver) | Apache 2.0 | (c) 2015-2025 Espressif Systems (Shanghai) CO LTD |

## License

This project is licensed under the **GNU General Public License v3.0** — see the [GPL-3.0 License](https://www.gnu.org/licenses/gpl-3.0.html) for details.

---

> [!NOTE]
> The sections below are kept for reference but are no longer actively maintained here. The [documentation site](https://teslaopencanmod.org/docs/intro) is the source of truth for installation, wiring, CAN details, and feature guides.

<details>
<summary>Legacy README — hardware details, installation, wiring, CAN tables</summary>
## What It Does

This firmware runs on an Adafruit Feather with CAN bus support (RP2040 CAN with MCP2515, M4 CAN Express with native ATSAME51 CAN, or any ESP32 board with a built-in TWAI peripheral). It intercepts specific CAN bus messages to enable and configure Full Self-Driving (FSD). Additionally, ASS (Actually Smart Summon) is no longer restricted by EU regulations.

Core Function

- Intercepts specific CAN bus messages
- Re-transmits them onto the vehicle bus

FSD Activation Logic

- Listens for Autopilot-related CAN frames
- Checks if "Traffic Light and Stop Sign Control" is enabled in the Autopilot settings Uses this setting as a trigger for Full Self-Driving (FSD)
- Adjusts the required bits in the CAN message to activate FSD

Additional Behavior

- Reads the follow-distance stalk setting
- Maps it dynamically to a speed profile

HW4 - FSD V14 Features

- Enhanced Autopilot (optional)
- Approaching Emergency Vehicle Detection

### Supported Hardware Variants

Select your vehicle hardware variant in `RP2040CAN/sketch_config.h` via the `#define` directive. Arduino IDE and PlatformIO both use the same board, vehicle, and feature defines from that file.

| Define   | Target           | Listens on CAN IDs | Notes |
|----------|------------------|---------------------|-------|
| `LEGACY` | HW3 Retrofit     | 1006, 69            | Sets FSD enable bit and speed profile control via follow distance |
| `HW3`    | HW3 vehicles     | 1016, 1021          | Same functionality as legacy |
| `HW4`    | HW4 vehicles     | 1016, 1021          | Extended speed-profile range (5 levels) |

> [!NOTE]
> HW4 vehicles on firmware **2026.2.9.X** are on **FSD v14**. However, versions on the **2026.8.X** branch are still on **FSD v13**. If your vehicle is running FSD v13 (including the 2026.8.X branch or anything older than 2026.2.9), compile with `HW3` even if your vehicle has HW4 hardware.

### How to Determine Your Hardware Variant

- **Legacy** — Your vehicle has a **portrait-oriented center screen** and **HW3**. This applies to older (pre Palladium) Model S and Model X vehicles that originally came with or were retrofitted with HW3.
- **HW3** — Your vehicle has a **landscape-oriented center screen** and **HW3**. You can check your hardware version under **Controls → Software → Additional Vehicle Information** on the vehicle's touchscreen.
- **HW4** — Same as above, but the Additional Vehicle Information screen shows **HW4**.

### Key Behaviour

- **FSD enable bit** is set when **"Traffic Light and Stop Sign Control"** is enabled in the vehicle's Autopilot settings.
- **Speed profile** is derived from the scroll-wheel offset or follow-distance setting.
- **Nag suppression** — clears the hands-on-wheel nag bit. On HW3/HW4 this is bundled into `ENHANCED_AUTOPILOT`.
- Debug output is printed over Serial at 115200 baud when `enablePrint` is `true`.

### CAN Message Details

The table below shows exactly which CAN messages each hardware variant monitors and what modifications are made.

> [!IMPORTANT]
> If a CAN message contains a counter or checksum, any modification to that message **must** recalculate these fields — otherwise the receiving ECU will discard the frame. Messages with these integrity fields require strict test coverage to ensure correctness.

#### Legacy (HW3 Retrofit)

| CAN ID | Name | R/W | Mux | Bit | Value | Signal | Description |
|---|---|---|---|---|---|---|---|
| 69 | STW_ACTN_RQ | R | — | 13–15 | (0–7) | | read stalk |
| 1006 | — | R+W | 0 | 38 | (0/1) | | read FSD |
| 1006 | — | R+W | 0 | 46 | 1 | | enable FSD |
| 1006 | — | R+W | 0 | 49–50 | (0–2) | | inject profile |
| 1006 | — | R+W | 1 | 19 | 0 | | suppress nag |

#### HW3

| CAN ID | Name | R/W | Mux | Bit | Value | Signal | Description |
|---|---|---|---|---|---|---|---|
| 1016 | UI_driverAssistControl | R | — | 45–47 | (0–7) | UI_accFollowDistanceSetting | read distance |
| 1021 | UI_autopilotControl | R+W | 0 | 38 | (0/1) | UI_fsdStopsControlEnabled | read FSD |
| 1021 | UI_autopilotControl | R+W | 0 | 25–30 | (offset) | | read offset |
| 1021 | UI_autopilotControl | R+W | 0 | 46 | 1 | | enable FSD |
| 1021 | UI_autopilotControl | R+W | 0 | 49–50 | (0–2) | | inject profile |
| 1021 | UI_autopilotControl | R+W | 1 | 19 | 0 | UI_applyEceR79 | suppress nag (`ENHANCED_AUTOPILOT`) |
| 1021 | UI_autopilotControl | R+W | 2 | 6–7, 8–13 | (offset) | | inject offset |

#### HW4

| CAN ID | Name | R/W | Mux | Bit | Value | Signal | Description |
|---|---|---|---|---|---|---|---|
| 921 | DAS_status | R+W | — | 13 | 1 | DAS_suppressSpeedWarning | suppress chime |
| 921 | DAS_status | R+W | — | 56–63 | (checksum) | DAS_statusChecksum | update checksum |
| 1016 | UI_driverAssistControl | R | — | 45–47 | (0–7) | UI_accFollowDistanceSetting | read distance |
| 1021 | UI_autopilotControl | R+W | 0 | 38 | (0/1) | UI_fsdStopsControlEnabled | read FSD |
| 1021 | UI_autopilotControl | R+W | 0 | 46 | 1 | | enable FSD |
| 1021 | UI_autopilotControl | R+W | 0 | 59 | 1 | | enable detection |
| 1021 | UI_autopilotControl | R+W | 0 | 60 | 1 | | enable V14 |
| 1021 | UI_autopilotControl | R+W | 1 | 19 | 0 | UI_applyEceR79 | suppress nag (`ENHANCED_AUTOPILOT`) |
| 1021 | UI_autopilotControl | R+W | 1 | 47 | 1 | UI_hardCoreSummon | enable summon (`ENHANCED_AUTOPILOT`) |
| 1021 | UI_autopilotControl | R+W | 2 | 60–62 | (0–4) | | inject profile |

> Signal names sourced from [tesla-can-explorer](https://github.com/mikegapinski/tesla-can-explorer) by [@mikegapinski](https://x.com/mikegapinski).

## Supported Boards

| Board                                                                   | CAN Interface              | Library                      | Status                             | Case STL |
|-------------------------------------------------------------------------|----------------------------|------------------------------|------------------------------------|----------|
| Adafruit Feather RP2040 CAN                                             | MCP2515 over SPI           | `mcp2515.h` (autowp)         | Tested                             | [Printables](https://www.printables.com/model/1662242-adafruit-rp2040-can-bus-feather-case-5724) |
| Adafruit Feather M4 CAN Express (ATSAME51)                              | Native MCAN peripheral     | `Adafruit_CAN` (`CANSAME5x`) | Tested                             |
| ESP32 with CAN transceiver (e.g. ESP32-DevKitC + SN65HVD230)            | Native TWAI peripheral     | ESP-IDF `driver/twai.h`      | Tested |
| [Atomic CAN Base](https://docs.m5stack.com/en/atom/Atomic%20CAN%20Base) | CA-IS3050G over ESP32 TWAI | ESP32 TWAI                   | Tested                             |
| M5Stack AtomS3 CAN Base                                                 | CA-IS3050G over ESP32-S3 TWAI | ESP32 TWAI                | Build target                       |

## Hardware Requirements

- One of the supported boards listed above
- CAN bus connection to the vehicle (500 kbit/s)

**Feather RP2040 CAN** — the board must expose these pins (defined by the earlephilhower board variant):

- `PIN_CAN_CS` — SPI chip-select for the MCP2515
- `PIN_CAN_INTERRUPT` — interrupt pin (unused; polling mode)
- `PIN_CAN_STANDBY` — CAN transceiver standby control
- `PIN_CAN_RESET` — MCP2515 hardware reset

**Feather M4 CAN Express** — uses the native ATSAME51 CAN peripheral; requires:

- `PIN_CAN_STANDBY` — CAN transceiver standby control
- `PIN_CAN_BOOSTEN` — 3V→5V boost converter enable for CAN signal levels

**ESP32 with CAN transceiver** — uses the native TWAI peripheral; requires:

- An external CAN transceiver module (e.g. SN65HVD230, TJA1050, or MCP2551)
- `TWAI_TX_PIN` — GPIO connected to the transceiver TX pin (default `GPIO_NUM_5`)
- `TWAI_RX_PIN` — GPIO connected to the transceiver RX pin (default `GPIO_NUM_4`)

**Adafruit ESP32 Feather V2 (5400) + Adafruit CAN Bus Featherwing (5709)** — uses the MCP2515 over SPI via the Featherwing stacked on top.

- Ensure the stacking headers are installed and the SPI bus is properly connected.
- `PIN_CAN_CS` — SPI chip-select for the MCP2515. Defaults to pin 14.
- `PIN_CAN_INTERRUPT` — interrupt pin (unused; polling mode). Defaults to pin 32.

> [!IMPORTANT]
> Cut the onboard 120 Ω termination resistor on the Feather CAN board (jumper labeled **TERM** on RP2040, **Trm** on M4). If using an ESP32 with an external transceiver that has a termination resistor, remove or disable it as well. The vehicle's CAN bus already has its own termination, and adding a second resistor will cause communication errors.

## Installation

### Option A: Arduino IDE — Flash Only

*Recommended if you just want to flash the firmware onto your board. No command-line tools required.*

#### 1. Install the Arduino IDE

Download from [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software).

#### 2. Add the Board Package

**For Feather RP2040 CAN:**

1. Open **File → Preferences**.
2. In **Additional Board Manager URLs**, add:

   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```

3. Go to **Tools → Board → Boards Manager**, search for **Raspberry PI Pico/RP2040**, and install it.
4. Select **Adafruit Feather RP2040 CAN** as the Board.

**For Feather M4 CAN Express:**

1. In **Additional Board Manager URLs**, add:

   ```
   https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
   ```

2. Install **Adafruit SAMD Boards** from the Boards Manager.
3. Select **Feather M4 CAN (SAME51)** as the Board.
4. Install the **Adafruit CAN** library via the Library Manager.

**For ESP32 boards:**

1. In **Additional Board Manager URLs**, add:

   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

2. Install **esp32 by Espressif Systems** from the Boards Manager.
3. Select your ESP32 board (e.g. **ESP32 Dev Module**).

#### 3. Install Required Libraries

Install via **Sketch → Include Library → Manage Libraries…**:

- **Feather RP2040 CAN:** MCP2515 by autowp
- **Feather M4 CAN Express:** Adafruit CAN
- **ESP32:** No additional libraries needed — the TWAI driver is built into the ESP32 Arduino core.

#### 4. Open the Arduino Sketch

Open the `RP2040CAN` folder in Arduino IDE, or open `RP2040CAN/RP2040CAN.ino` directly. The sketch folder name matches the primary `.ino`, which keeps Arduino IDE happy.
The sketch also exposes the shared headers through `RP2040CAN/src`, so Arduino IDE builds the same shared code that PlatformIO uses.

#### 5. Select Your Board and Vehicle

In the `sketch_config.h` tab, uncomment the line that matches your **board**:

```cpp
#define DRIVER_MCP2515   // Adafruit Feather RP2040 CAN (MCP2515 over SPI)
//#define DRIVER_SAME51  // Adafruit Feather M4 CAN Express (native ATSAME51 CAN)
//#define DRIVER_TWAI    // ESP32 boards with built-in TWAI (CAN) peripheral
```

Then uncomment the line that matches your **vehicle**:

```cpp
#define LEGACY // HW4, HW3, or LEGACY
//#define HW3
//#define HW4
```

You can also enable optional features in the same file:

```cpp
// #define ISA_SPEED_CHIME_SUPPRESS
// #define EMERGENCY_VEHICLE_DETECTION
// #define BYPASS_TLSSC_REQUIREMENT
// #define ENHANCED_AUTOPILOT               // Summon + UI_applyEceR79 override on HW4
```

#### 6. Upload

1. Connect the Feather via USB.
2. Select the correct board and port under **Tools**.
3. Click **Upload**.

### Option B: PlatformIO — Development, Testing & Flash

*For developers who want to run unit tests, build for multiple boards, or integrate with CI. Can also flash firmware to the board.*

#### Prerequisites (Windows)

| Tool | Purpose | Install |
|------|---------|---------|
| [Python 3](https://www.python.org/downloads/) | PlatformIO runtime | `winget install Python.Python.3.14` |
| [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) | Build system & test runner | `pip install platformio` |
| [MinGW-w64 GCC](https://winlibs.com/) | Native test compiler | `winget install BrechtSanders.WinLibs.POSIX.UCRT` |

> [!TIP]
> After installing MinGW-w64, restart your terminal so `gcc` and `g++` are on PATH. GCC is only needed for `pio test -e native` (host-side unit tests) — cross-compiling to the Feather boards uses PlatformIO's built-in ARM toolchain.

#### Build

1. Select your board, vehicle, and optional features in `RP2040CAN/sketch_config.h`:

   ```cpp
   #define DRIVER_MCP2515  // Change to DRIVER_SAME51 or DRIVER_TWAI for other boards
   #define HW4             // Change to HW4, HW3, or LEGACY
   #define EMERGENCY_VEHICLE_DETECTION  // Optional
   #define ENHANCED_AUTOPILOT           // Optional
   ```

   PlatformIO reads the active board, vehicle, and optional feature defines from `RP2040CAN/sketch_config.h`.
   The `-e` environment still selects the board, so it must match the uncommented driver define in the shared config. If they do not match, the build stops with a clear error.

   You can switch the shared sketch profile from the command line instead of editing the file by hand:

   ```bash
   python3 scripts/platformio_set_ino_profile.py --driver DRIVER_MCP2515 --vehicle HW4 --enable EMERGENCY_VEHICLE_DETECTION --enable ENHANCED_AUTOPILOT
   ```

2. Build for your board:

   ```bash
   # Adafruit Feather RP2040 CAN
   pio run -e feather_rp2040_can

   # Adafruit Feather M4 CAN Express (ATSAME51)
   pio run -e feather_m4_can

   # ESP32 with TWAI (CAN) peripheral
   pio run -e esp32_twai

   # M5Stack Atomic CAN Base
   pio run -e m5stack-atomic-can-base

   # M5Stack AtomS3 CAN Base
   pio run -e m5stack-atoms3-can-base
   ```

#### Flash

Connect the board via USB, then upload:

```bash
# Adafruit Feather RP2040 CAN
pio run -e feather_rp2040_can --target upload

# Adafruit Feather M4 CAN Express (ATSAME51)
pio run -e feather_m4_can --target upload

# ESP32
pio run -e esp32_twai --target upload

# M5Stack Atomic CAN Base
pio run -e m5stack-atomic-can-base --target upload

# M5Stack AtomS3 CAN Base
pio run -e m5stack-atoms3-can-base --target upload
```

> [!TIP]
> For Feather boards, if the board is not detected, double-press the **Reset** button to enter the UF2 bootloader, then retry the upload command. For ESP32 boards, hold the **BOOT** button during upload if auto-reset does not work.

#### Run Tests

Unit tests run on your host machine — no hardware required:

```bash
pio test -e native
```

Additional native test targets cover the `BYPASS_TLSSC_REQUIREMENT` path and the standalone log buffer suite:

```bash
pio test -e native_bypass_tlssc_requirement
pio test -e native_log_buffer
```

> [!TIP]
> On macOS, the native PlatformIO test environments automatically add the Xcode Command Line Tools libc++ headers. If `xcode-select -p` fails, install the Command Line Tools first.

### Wiring

The recommended connection point is the [**X179 connector**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-233/connector/x179/):

| Pin | Signal |
|-----|--------|
| 13  | CAN-H  |
| 14  | CAN-L  |

Connect the Feather's CAN-H and CAN-L lines to pins 13 and 14 on the X179 connector.

The recommended connection point for **legacy Model 3 (2020 and earlier)** is the [**X652 connector**](https://service.tesla.com/docs/Model3/ElectricalReference/prog-187/connector/x652/) if the vehicle is not equipped with the X179 port (varies depending on production date):

| Pin | Signal |
|-----|--------|
| 1  | CAN-H  |
| 2  | CAN-L  |

Connect the Feather's CAN-H and CAN-L lines to pins 1 and 2 on the X652 connector.

## Speed Profiles

The speed profile controls how aggressively the vehicle drives under FSD. It is configured differently depending on the hardware variant:

### Legacy, HW3 & HW4 Profiles

The "Distance" column below is the raw `UI_accFollowDistanceSetting`
CAN signal value (3-bit field on CAN 1016 byte 5 bits 5-7). The Tesla
in-cluster display number may differ by one depending on firmware.

| Distance | Profile (HW3) | Profile (HW4) |
| :--- | :--- | :--- |
| 1 | Hurry  | Max   |
| 2 | Normal | Max   |
| 3 | Chill  | Max   |
| 4 | —      | Hurry |
| 5 | —      | Normal|
| 6 | —      | Chill |
| 7 | —      | Sloth |

> HW4 mapping is customized: stalk values 1-3 all latch onto **Max**
> (so the often-clamped physical minimum still hits the most aggressive
> profile), and 4-7 walk monotonically slower across Hurry → Normal →
> Chill → Sloth.

## Serial Monitor

Open the Serial Monitor at **115200 baud** to see live debug output showing FSD state and the active speed profile. Disable logging by setting `enablePrint = false`.

## Board Porting Notes

The project uses an abstract `CanDriver` interface so that all vehicle logic (handlers, bit manipulation, speed profiles) is shared across boards. Only the driver implementation changes.

**What changes per board:**

- **RP2040 CAN:** `mcp2515.h` (autowp) — SPI-based, struct read/write, needs `PIN_CAN_CS`
- **M4 CAN Express:** `Adafruit_CAN` (`CANSAME5x`) — native MCAN peripheral, packet-stream API, needs `PIN_CAN_BOOSTEN`
- **ESP32 TWAI:** ESP-IDF `driver/twai.h` — native TWAI peripheral, FreeRTOS queue-based RX, needs an external CAN transceiver and two GPIO pins

**What stays identical:**

- All handler structs and bit manipulation logic
- Vehicle-specific behavior (FSD enable, nag suppression, speed profiles)
- Serial debug output

## Development & Testing

The project uses [PlatformIO](https://platformio.org/) with the [Unity](https://github.com/ThrowTheSwitch/Unity) test framework.

### Project Structure

```
include/
  can_frame_types.h       # Portable CanFrame struct
  can_driver.h            # Abstract CanDriver interface
  can_helpers.h           # setBit, readMuxID, isFSDSelectedInUI, setSpeedProfileV12V13
  handlers.h              # CarManagerBase, LegacyHandler, HW3Handler, HW4Handler
  app.h                   # Shared setup/loop logic for all entry points
  arduino_entrypoint.h    # Shared Arduino setup/loop entry point
  drivers/
    mcp2515_driver.h      # MCP2515 driver (Feather RP2040 CAN)
    same51_driver.h       # CANSAME5x driver (Feather M4 CAN Express)
    twai_driver.h         # ESP32 TWAI driver
    mock_driver.h         # Mock driver for unit tests
RP2040CAN/
  RP2040CAN.ino           # Arduino IDE sketch entry point
  sketch_config.h         # Shared board, vehicle, and feature defines
  src/                    # Shared headers exposed inside the sketch for Arduino IDE
src/
  main.cpp                # PlatformIO entry point
scripts/
  platformio_set_ino_profile.py   # Switch shared board/vehicle/feature defines
  platformio_sync_ino_defines.py  # Sync shared sketch defines into PlatformIO envs
  platformio_native_env.py        # Add macOS native test compiler includes
test/
  test_native_helpers/    # Tests for bit manipulation helpers
  test_native_legacy/     # LegacyHandler tests
  test_native_hw3/        # HW3Handler tests
  test_native_hw4/        # HW4Handler tests
  test_native_twai/       # TWAI filter computation tests
```

### Continuous Integration

GitLab CI validates three layers:

- `pio run` for `feather_rp2040_can`, `feather_m4_can`, `esp32_twai`, and `m5stack-atomic-can-base`
- `pio test -e native`, `pio test -e native_bypass_tlssc_requirement`, and `pio test -e native_log_buffer`
- `pio run` for `feather_rp2040_can`, `feather_m4_can`, `esp32_twai`, `m5stack-atomic-can-base`, and `m5stack-atoms3-can-base`
- `arduino-cli compile` for the `RP2040CAN` sketch folder on `rp2040:rp2040:adafruit_feather_can`

The GitLab pipeline in `.gitlab-ci.yml` rewrites `RP2040CAN/sketch_config.h` per job using `scripts/platformio_set_ino_profile.py`, so the shared Arduino sketch profile stays the source of truth in CI as well.

### Running Tests

```bash
pio test -e native
```

Tests run on your host machine — no hardware required. They cover all handler logic including FSD activation, nag suppression, speed profile mapping, and bit manipulation correctness.

</details>
