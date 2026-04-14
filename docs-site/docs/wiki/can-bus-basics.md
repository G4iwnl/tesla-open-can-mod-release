---
sidebar_position: 4
---

# CAN Bus Basics

A brief introduction to the CAN (Controller Area Network) bus and how it relates to this project.

## What is CAN?

CAN (Controller Area Network) is a serial communication protocol used in vehicles and industrial systems. In a Tesla, the CAN bus connects all Electronic Control Units (ECUs) — the touchscreen, autopilot computer, body controller, battery management, and more.

## Key Concepts

### Messages and IDs

Every CAN message has an **ID** (an integer) and a **data payload** (up to 8 bytes). ECUs filter messages by ID — they only process the IDs they care about.

This project monitors specific CAN IDs:

| CAN ID | Name | Purpose |
|---|---|---|
| 69 | STW_ACTN_RQ | Steering wheel stalk actions (Legacy) |
| 921 | DAS_status | Driver Assistance System status (HW4) |
| 1016 | UI_driverAssistControl | Follow distance setting (HW3/HW4) |
| 1021 | UI_autopilotControl | FSD enable, nag, speed profiles |
| 1006 | — | FSD enable, nag, speed profiles (Legacy) |

### Multiplexing (Mux)

Some CAN messages use **multiplexing** — the same CAN ID carries different data depending on a mux field in the payload. For example, CAN ID 1021 has mux values 0, 1, and 2, each carrying different signals.

### Counters and Checksums

Some CAN messages include a **counter** (incrementing sequence number) and a **checksum** (integrity check). When modifying these messages, the counter and checksum must be recalculated — otherwise the receiving ECU will discard the frame.

### Bit Manipulation

CAN payloads are binary data. Individual features are controlled by specific **bits** within the payload. For example, the FSD enable flag is a single bit at a specific position in CAN ID 1021.

## CAN Bus Speed

Tesla vehicles use a CAN bus speed of **500 kbit/s**. The firmware configures this automatically for all supported boards.

## Termination

A CAN bus requires exactly two 120 Ohm termination resistors — one at each end of the bus. The vehicle already has these. Adding a third termination resistor (from your board) will cause communication errors. That's why you must remove/cut any onboard termination resistor on your CAN board.

## Safety

:::danger
The CAN bus controls **everything** in the vehicle — braking, steering, airbags, throttle, lights, doors. A malformed CAN message can have serious, safety-critical consequences. Only modify CAN messages if you fully understand what you're doing.
:::

## Further Reading

- Signal names sourced from [tesla-can-explorer](https://github.com/mikegapinski/tesla-can-explorer) by @mikegapinski
