# Security & Responsible Use

## Disclaimer

> **FSD is a premium Tesla feature and must be properly purchased or subscribed to.** This project intercepts and modifies UI configuration frames at the CAN bus level. It does not bypass any cryptographic entitlement check on Tesla's servers, and using it without an active subscription is a violation of Tesla's Terms of Service.

> **Modifying CAN bus messages can cause dangerous behaviour or permanently damage your vehicle.** The CAN bus carries braking, steering, airbag, and powertrain control signals. A malformed or out-of-spec frame can have serious physical consequences.

> **Tesla has begun issuing VIN-level bans (April 2026).** Affected vehicles lose the TLSSC toggle silently — no OTA update, no warning. The ban persists across account transfers, FSD re-subscriptions, and software reinstalls. CAN injection cannot override a VIN-level ban. Non-FSD features (nag killer, battery dashboard, diagnostics) are not affected.

This project is published for testing, research, and educational purposes only. It is intended for use on **private property** and off public roads unless you have your own legal opinion that operating it on a public road is permitted in your jurisdiction.

The authors and contributors accept no responsibility for:

- Damage to your vehicle, including warranty voiding
- Personal injury or property damage
- Tesla account suspension or service revocation
- Violation of road traffic regulations in your country
- Civil or criminal liability arising from any of the above

## Reporting a Security Issue

If you find:

- A way for this project to corrupt or destabilize a CAN bus beyond what the documented behaviour does
- A buffer overflow, out-of-bounds access, or memory corruption in any firmware code
- A subtle frame interaction that could cascade into a vehicle safety fault
- A vulnerability in the web server (e.g. injection, authentication bypass, denial of service)

**Please do not open a public GitHub issue.** Email the maintainer privately with the subject `[security] tesla-open-can-mod` and describe the issue, the reproduction steps, and the affected version. We will respond within a few days and credit you in the patch release notes if you'd like.

For non-security bugs, the regular [issue tracker](https://github.com/ylovex75/tesla-open-can-mod-release/issues) is fine.

## What This Project Writes to the CAN Bus

The TX surface is intentionally narrow. Every `twai_transmit()` call site is documented below:

| CAN ID | Frame Name | What Is Modified | Condition |
|--------|-----------|------------------|-----------|
| `0x3FD` | UI_autopilotControl | Bits 19, 46, 47, 59, 60 + speed offset + profile | HW3/HW4 handler active, frame received from Gateway |
| `0x3EE` | UI_autopilotControl | Bits 19, 46 + profile | Legacy handler active, frame received from Gateway |
| `0x313` | TrackModeRequest | Byte 0 bits 0-1, checksum | HW3 handler active, frame received |
| `0x370` | EPAS3P_sysStatus | handsOnLevel=1, torque=1.80 Nm, counter+1, checksum | Nag Killer enabled AND handsOnLevel=0 detected |
| `0x399` | DAS_status | Byte 1 bit 5, checksum | ISA chime suppress enabled, frame received |
| `0x082` | UI_tripPlanning | Byte 0 = 0x05 (full frame injected) | Preheat enabled, 500 ms interval, single-shot TX |

**This project does NOT write to:**

- Brake controllers (`0x244` IBST_status and related)
- Steering controllers (`0x129` SteeringAngleSensor)
- Powertrain / motor (`0x118` DI_systemStatus, `0x132` BMS, `0x214` DI_torque)
- ESP / stability control (`0x2A1` ESP_status)
- Door / window / lock actuators (`0x102`, `0x3E3`)
- Transmission / gear selection (no known writable signal on Party CAN)
- Anything on Chassis CAN — we only sit on Vehicle / Party CAN (Bus 0)

The BMS, speed, OTA-detect, and follow-distance handlers are **read-only parsers**. They update internal state but never call `twai_transmit()`.

## Safety Mechanisms

### OTA Vehicle Protection

When the firmware detects a vehicle OTA update in progress (`GTW_carState` 0x318, bits 6-7 > 0), **all CAN frame modifications and injections are paused**. This prevents interference with the vehicle's own firmware update process. Modifications resume automatically when the OTA completes. The event is logged to the ring buffer.

### Preheat Safeguards

Battery preconditioning injection is the only fully-synthetic frame this project sends. Multiple safeguards exist:

- **Auto-stop by temperature:** Configurable threshold (-5°C to 25°C, default 10°C). Stops when `bmsTempMin >= threshold`.
- **Auto-stop by duration:** Configurable maximum (10–60 min, default 30 min).
- **No NVS persistence:** Preheat is always OFF on reboot. The user must manually re-enable it, preventing accidental silent re-activation.
- **Single-shot TX:** Frames use `TWAI_MSG_FLAG_SS` to prevent retry storms if the bus does not ACK.
- **OTA interlock:** Preheat injection is paused during vehicle OTA (see above).

### Speed Offset Safety

- A confirmation modal is shown on the first non-zero speed offset to warn the user that offset causes the vehicle to exceed posted speed limits.
- Manual and Smart modes are visually separated in the UI to prevent confusion.
- Smart offset rules are capped at 8 entries and auto-sorted by speed threshold.

### NVS Corruption Recovery

On boot, NVS initialization checks for `ESP_ERR_NVS_NO_FREE_PAGES` and `ESP_ERR_NVS_NEW_VERSION_FOUND`. If either is detected, the NVS partition is erased and reinitialized with default values. The event is logged.

### CAN Bus Isolation

- The firmware only accesses the **Party CAN bus** (Bus 0) via OBD-II or X179 connector. It has no physical access to Chassis CAN, Powertrain CAN, or the Ethernet backbone.
- All frame modifications are read-modify-retransmit on existing frames from the Gateway. The firmware does not generate arbitrary CAN IDs.
- The only exception is `0x082` (preheat injection), which is a known Tesla frame format sent with single-shot flag.

## Web Server Security

### Network Exposure

- The web server listens on port 80 (HTTP) within the ESP32's WiFi AP network (`192.168.4.x`).
- Default AP credentials: SSID `TeslaCAN`, password `12345678` (WPA2).
- **Change the WiFi password** if you operate in an environment where others could connect to the AP.
- The web server is not exposed to the internet unless you explicitly bridge the ESP32's network to a WAN.

### Rate Limiting

All POST endpoints enforce a 500 ms minimum interval. Requests within the cooldown return HTTP 429. This prevents:

- Accidental rapid toggling of safety-critical features
- Simple denial-of-service against the embedded web server

### Input Validation

- WiFi SSID: max 32 characters. Password: min 8, max 63 characters.
- Speed offset values are clamped to valid CAN ranges (0–200 in steps of 4).
- Smart offset rules are validated (maxSpeed > 0, offsetPct 0–100, max 8 rules).
- Hardware mode is clamped to 0–2 (Legacy/HW3/HW4).
- OTA upload validates firmware binary integrity; optional MD5 checksum via `X-Firmware-MD5` header.

### No Authentication

The web server does **not** implement authentication. Anyone connected to the WiFi AP has full control. This is acceptable for a single-user embedded device on a private AP, but be aware:

- Do not connect the ESP32 to a shared or public WiFi network in STA mode if you have not configured additional network-level access control.
- Do not expose port 80 to the internet.

## Detection Risk

Tesla's telemetry can detect anomalous CAN frame patterns. Known detection vectors include:

1. **Constant torque signature** — The nag killer echo uses a fixed 1.80 Nm torque value. A constant `torsionBarTorque` for extended periods is statistically impossible from a real hand and may be flagged.
2. **Bit 46 injection on 0x3FD** — The FSD enable bit is the primary modification. Tesla's server-side analysis can identify this pattern.
3. **Frame timing anomalies** — Retransmitted frames have slightly different timing than originals, which telemetry may detect.

### Risk Mitigation

- **Pull the SIM card** before use (Model 3/Y: behind the glovebox). This prevents real-time telemetry upload.
- **Disable WiFi** on the vehicle (Settings → WiFi → Forget all networks).
- **Note:** Even with SIM pulled, Tesla may flag VINs retroactively from historical telemetry data. There is no guaranteed way to avoid detection.
- **Non-FSD features** (battery dashboard, speed display, drive data recording) operate in read-only mode and do not modify CAN frames — they carry no detection risk.

## Recommended Pre-Flight

Before each session:

1. Pull the SIM card from the vehicle
2. Disable WiFi on the vehicle
3. Power on the device, verify CAN RX counter is incrementing (confirms wiring)
4. Check the web dashboard for sensible signal readings (speed, SOC, gear)
5. Enable features as needed

After each session:

- Disable active features or unplug the device
- Re-insert SIM and re-enable WiFi if needed for navigation / streaming

## License

This project is licensed under GPL-3.0. See [LICENSE](LICENSE) for details.
