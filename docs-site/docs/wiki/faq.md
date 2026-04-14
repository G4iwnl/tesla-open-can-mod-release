---
sidebar_position: 1
---

# FAQ

## General

### Do I need an FSD subscription?

**Yes.** You must have an active FSD package on the vehicle — either purchased or subscribed. The board enables FSD at the CAN bus level, but the vehicle still needs a valid FSD entitlement from Tesla.

### Is FSD subscription not available in my region?

You can work around this by creating a Tesla account in a region where FSD subscriptions are offered (e.g. Canada), transferring the vehicle to that account, and subscribing there. This costs approximately 99 CAD/month (~60 EUR). See the [FSD Subscription Guide](/docs/wiki/faq#fsd-subscription-from-outside-supported-regions) below.

### Will this void my warranty?

Modifying the CAN bus may void parts of your vehicle warranty. You are responsible for understanding the risks.

### Is this legal?

This project is intended for testing purposes only and for use on private property. For any use beyond private testing, you are responsible for complying with all applicable local laws and regulations.

## Hardware

### Which board should I buy?

The **Adafruit Feather RP2040 CAN** is the most popular and well-documented choice. See [Hardware Selection](/docs/getting-started/hardware-selection) for a full comparison.

### Do I need to cut the termination resistor?

**Yes.** All Feather CAN boards and many external CAN transceivers ship with a 120 Ohm termination resistor. The vehicle's CAN bus already has its own termination — adding a second one causes communication errors.

### Where do I connect the board in the car?

The recommended connection point is the **X179 connector** in the passenger side footwell. See the [Installation Guide](/docs/getting-started/installation) for details.

## Software

### How do I know which vehicle variant to select?

- **Legacy:** Portrait-oriented center screen + HW3 (pre-Palladium Model S/X)
- **HW3:** Landscape-oriented center screen + HW3
- **HW4:** Landscape-oriented center screen + HW4

Check **Controls > Software > Additional Vehicle Information** on the touchscreen.

### My HW4 vehicle is on FSD v13 — which variant do I compile with?

Compile with `HW3`. HW4 vehicles on the 2026.8.X branch or anything older than 2026.2.9 are still on FSD v13.

### What does BYPASS_TLSSC_REQUIREMENT do?

Normally, FSD is only activated when "Traffic Light and Stop Sign Control" is toggled on. `BYPASS_TLSSC_REQUIREMENT` bypasses this check and always enables FSD.

## FSD Subscription from Outside Supported Regions

1. Go to the Tesla website, select **Canada/English** as region
2. Create a new Tesla account
3. Transfer your vehicle to the Canadian account via the Tesla app (My Products > Remove or Transfer Ownership)
4. Log into the Tesla app with the Canadian account
5. Go to **Upgrades** and subscribe to **Full Self-Driving (Supervised)**

Your vehicle now has a valid FSD entitlement.
