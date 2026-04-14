---
title: Autosteer Nag Killer
sidebar_position: 3
---

# Autosteer Nag Killer

Suppresses the Autopilot "hands on wheel" nag on Tesla vehicles using the same method as the Chinese TSL6P commercial module (~200 EUR), replicated on a ~15 EUR ESP32 board.

:::info Difference from ISA Speed Chime
- **Nag Suppression (ISA Speed Chime)**: Modifies CAN 39F message (controls speed chime alerts)
- **Autosteer Nag Killer**: Modifies CAN 880 message (suppresses "hands on wheel" nag)

These are two different features addressing different nag/chime issues.
:::

## How It Works

The module echoes CAN 880 (EPAS3P_sysStatus) frames with a modified payload:

1. Listens for CAN 880 frames where `handsOnLevel = 0` (driver hands not detected)
2. Immediately retransmits the frame with:
   - `byte 3 = 0xB6` → fixed torsionBarTorque = 1.80 Nm
   - `byte 4 |= 0x40` → handsOnLevel = 1
   - `byte 6 counter + 1` → next expected counter value
   - `byte 7` → recalculated checksum: `(sum(byte0..byte6) + 0x73) & 0xFF`
3. The real EPAS frame with the same counter arrives after ours → receiver rejects it as duplicate
4. When driver applies real steering torque (handsOn >= 1), the module does nothing

### Frame Example

```
Real EPAS:      12 00 08 23 1F 89 4C A4   (ho=0, torque=+0.33Nm, cnt=C)
Our modified:   12 00 08 B6 5F 89 4D 78   (ho=1, torque=+1.80Nm, cnt=D)
```

## Tested Hardware

| Vehicle | HW | Autopilot | Status |
|---------|-----|-----------|--------|
| Model Y Performance 2022 | HW3 | Basic Autopilot | **Working** |

## Supported Boards

**Any ESP32 board with a CAN transceiver** will work. Tested with:

- **LILYGO T-CAN485** (~15 EUR) — ESP32 + SN65HVD230 CAN transceiver, screw terminals

**Other compatible boards:**
- ESP32 + MCP2551 or SN65HVD230 breakout
- M5Stack ATOM CAN
- Any board supported by this project

## Wiring to Vehicle

Connect to **X179 diagnostic connector, pin 2 and pin 3** (CAN bus 4):

```
Board CAN_H  ──────  X179 Pin 2  (CAN bus 4+)
Board CAN_L  ──────  X179 Pin 3  (CAN bus 4-)
Board USB    ──────  Any 5V source (car USB, phone charger, laptop)
```

### X179 20-pin Connector Pinout (S/X new Blue / Model Y)

```
     ┌─────────────────────────────┐
     │  1   2   3   4   5   6   7  │  ← Top row (pin 1 = 12V+)
     │  8   9  10  11  12  13  14  │  ← Middle row
     │ 15  16  17  18  19  20      │  ← Bottom row (pin 20 = GND)
     └─────────────────────────────┘
            (front mating face)
```

| Pin | Function | Notes |
|-----|----------|-------|
| 1 | 12V+ | Vehicle power |
| **2** | **CAN bus 4+** | **← Connect CAN_H here** |
| **3** | **CAN bus 4-** | **← Connect CAN_L here** |
| 6 | K/Serial | |
| 9 | CAN bus 2+ | Vehicle CAN |
| 10 | CAN bus 2- | Vehicle CAN |
| 13 | CAN bus 6+ | Body/Left CAN (used by FSD mod / Enhance cable) |
| 14 | CAN bus 6- | Body/Left CAN |
| 18 | CAN bus 3+ | Chassis CAN |
| 19 | CAN bus 3- | Chassis CAN |
| 20 | Ground | |

:::warning Important: CAN Bus 4 Only
The nag killer works **ONLY on CAN bus 4** (pin 2/3). It does NOT work on CAN bus 3 (pin 18/19) or CAN bus 6 (pin 13/14) due to Tesla's anti-spoofing on those buses.

Pin numbering: Always read pin numbers from the **FRONT MATING FACE** of the connector. If you look at the back (wire side) of a female connector, the numbers appear mirrored!
:::

## Building & Flashing

### With PlatformIO (Recommended)

For LILYGO T-CAN485:
```bash
pio run -e lilygo_tcan485_nag_killer
pio run -e lilygo_tcan485_nag_killer -t upload
```

For generic ESP32 + CAN transceiver (default pins GPIO 5/4):
```bash
pio run -e esp32_twai_nag_killer
pio run -e esp32_twai_nag_killer -t upload
```

### With Arduino IDE / CLI

Use build flag `-DNAG_KILLER` and set your board's TX/RX pins with `-DTWAI_TX_PIN=GPIO_NUM_xx -DTWAI_RX_PIN=GPIO_NUM_xx`.

## LILYGO T-CAN485 Pin Mapping

| Function | GPIO |
|----------|------|
| CAN TX | 27 |
| CAN RX | 26 |
| 5V Enable | 16 |
| CAN Standby | 23 (LOW = active) |

## What Doesn't Work (Extensively Tested)

| Approach | Bus | Why it fails |
|----------|-----|-------------|
| CAN 880 echo | Bus 3 (Chassis, pin 18/19) | Gateway anti-spoofing blocks |
| CAN 880 echo | Bus 6 (Body, pin 13/14) | DAS ignores injected frames |
| CAN 82 echo | Bus 6 (Body, pin 13/14) | DAS ignores |
| CAN 905 echo | Bus 3 (Chassis) | Source validation rejects |
| CAN 1160 LKA torque | Bus 3 (Chassis) | DAS sends at 50Hz, overrides |
| CAN 82 injection | Bus 3 (Chassis) | No effect |

**Only CAN bus 4 (pin 2/3) works** — it does not have the same counter/source validation as the other buses.

## Safety

:::caution Safety Guidelines
- The module only activates when `handsOnLevel = 0` (hands not detected)
- When you grip the wheel normally, the module does nothing
- CAN bus-off auto-recovery is built in
- No vehicle modifications — fully reversible, plug and play
- **Keep your hands near the wheel and stay attentive at all times**
:::

## Credits

- Reverse engineered by BatteryPlug (nicolozak) with Claude AI assistance
- TSL6P method decoded with help from the Tesla Open CAN Mod Discord community
- DBC files from Tesla community (Poppyseed, Onyx)
