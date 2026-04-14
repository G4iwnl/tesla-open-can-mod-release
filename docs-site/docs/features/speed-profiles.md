---
sidebar_position: 3
---

# Speed Profiles

The speed profile controls how aggressively the vehicle drives under FSD. It is derived from the follow-distance stalk setting and injected into the CAN message.

## Profile Mapping

The "Follow Distance" column below is the raw
`UI_accFollowDistanceSetting` CAN signal value (3-bit field on CAN 1016
byte 5 bits 5-7). The in-cluster display number may differ by one
depending on firmware.

### Legacy & HW3

| Follow Distance | Profile |
|---|---|
| 1 | Hurry  |
| 2 | Normal |
| 3 | Chill  |

### HW4 (customized)

HW4 supports an extended 5-level speed profile range. This fork latches
stalk values 1-3 onto **Max** so the often-clamped physical minimum
still hits the most aggressive profile, then walks monotonically slower
across the rest:

| Follow Distance | Profile |
|---|---|
| 1 | Max    |
| 2 | Max    |
| 3 | Max    |
| 4 | Hurry  |
| 5 | Normal |
| 6 | Chill  |
| 7 | Sloth  |

## CAN Message Details

### Reading the Follow Distance

| Variant | CAN ID | Signal | Bits | Values |
|---|---|---|---|---|
| Legacy | 69 (STW_ACTN_RQ) | Stalk | 13-15 | 0-7 |
| HW3 | 1016 (UI_driverAssistControl) | UI_accFollowDistanceSetting | 45-47 | 0-7 |
| HW4 | 1016 (UI_driverAssistControl) | UI_accFollowDistanceSetting | 45-47 | 0-7 |

### Injecting the Profile

| Variant | CAN ID | Mux | Bits | Values |
|---|---|---|---|---|
| Legacy | 1006 | 0 | 49-50 | 0-2 |
| HW3 | 1021 | 0 | 49-50 | 0-2 |
| HW4 | 1021 | 2 | 60-62 | 0-4 |

## How to Change the Speed Profile

While driving with FSD active, use the **follow distance** control on the steering wheel stalk to cycle through profiles. The change takes effect immediately.
