---
sidebar_position: 1
slug: /intro
---

# Tesla Open CAN Mod

An open-source general-purpose CAN bus modification tool for Tesla vehicles. While FSD enablement was the starting point, the goal is to expose and control everything accessible via CAN — as a fully open project.

Some sellers charge up to 500 EUR for a solution like this. The hardware costs around 20 EUR, and even with labor factored in, a fair price is no more than 50 EUR. This project exists so nobody has to overpay.

## What It Does

This firmware runs on supported microcontroller boards with CAN bus support. It intercepts specific CAN bus messages to enable and configure Full Self-Driving (FSD).

### Core Function

- Intercepts specific CAN bus messages
- Re-transmits them onto the vehicle bus

### FSD Activation Logic

- Listens for Autopilot-related CAN frames
- Checks if "Traffic Light and Stop Sign Control" is enabled in the Autopilot settings
- Uses this setting as a trigger for Full Self-Driving (FSD)
- Adjusts the required bits in the CAN message to activate FSD

### Additional Behavior

- Reads the follow-distance stalk setting
- Maps it dynamically to a speed profile
- Nag suppression — clears the hands-on-wheel nag bit
- HW4: Enhanced Autopilot (optional)
- HW4: Approaching Emergency Vehicle Detection
- HW4: Actually Smart Summon (ASS) without EU restrictions

## Prerequisites

**You must have an active FSD package on the vehicle** — either purchased or subscribed. This board enables the FSD functionality on the CAN bus level, but the vehicle still needs a valid FSD entitlement from Tesla.

:::danger Ban Warning
Any attempt to bypass the purchase or subscription requirement for Full Self-Driving (FSD) will result in a permanent ban from Tesla services. FSD is a premium feature and must be properly purchased or subscribed to.
:::

:::warning
This project is for testing and educational purposes only. Sending incorrect CAN bus messages to your vehicle can cause unexpected behavior, disable safety-critical systems, or permanently damage electronic components. The CAN bus controls everything from braking and steering to airbags — a malformed message can have serious consequences. If you don't fully understand what you're doing, **do not install this on your car**.
:::

## Next Steps

- **[Hardware Selection](/docs/getting-started/hardware-selection)** — Choose the right board for your vehicle
- **[Installation](/docs/getting-started/installation)** — Wire your board to the car
- **[Firmware Flash](/docs/getting-started/firmware-flash)** — Flash the firmware to your board
- **[Configuration](/docs/getting-started/configuration)** — Configure board, vehicle, and features
