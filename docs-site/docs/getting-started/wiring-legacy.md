---
sidebar_position: 2.5
---

# Wiring Guide: Legacy (Pre-2020) Tesla Model 3/Y

For older Tesla Model 3/Y vehicles (2020 and earlier), there are different connector options available. This guide covers the X652 and X052 connectors.

## Finding the Right Connector for Your Car

Please use the official Tesla documentation for reference: https://service.tesla.com/docs/Model3/ElectricalReference/

## Option 1: X652 Connector (Easiest)

The X652 connector is the easiest to use if your car has it.

### Locating X652

Lift the passenger seat footwell all the way up and look under the seat:

![X625 location](/img/x625-image.png)

### Important: Check for Black Box

**Do NOT use the X652 if it is connected to a black plastic box.** Unplugging it will trigger a persistent passenger seat warning.

If there is no black box, you can safely use this connector.

### Wiring X652

![X625 pins](/img/x625-pins.png)

| Pin | Signal |
|---|---|
| 1 | CAN-L |
| 2 | CAN-H |
| 3 | Ground |
| 4 | Power (12V) |

You'll need to make 2 custom cables using:
- [936119 connectors](https://aliexpress.com/w/wholesale-936119%2525252d1.html) for CAN bus
- [1355717 connectors](https://aliexpress.com/w/wholesale-1355717%2525252d1.html)
- [Power connector](https://aliexpress.com/item/1005008884473620.html)

## Option 2: X052 Connector

If X652 is not available, you can use the X052 connector located on the right side of the car.

### Locating X052

![Right side car model 3](/img/rightsidecarmodel3.png)

There is 1 plastic trim piece that needs to be removed:
1. Pull the trim up near the bottom
2. Lift it slowly toward the dashboard
3. Unclip the plastic screw by hand

You will find the X052 connector inside:

![X052 location](/img/x052-location.png)

### X052 Connector

![X052](/img/x052.png)

The X052 uses special deep-plugging connectors. We recommend buying [these connectors](https://aliexpress.com/item/1005008523245081.html) and making 4 cables.

| Pin | Signal |
|---|---|
| 44 | CAN-H |
| 45 | CAN-L |
| 22 | Ground |
| 20 | Power (12V) |

## Option 3: X930M (Not Recommended)

The X930M connector **will not enable FSD** and is not recommended for this project.

## Next Steps

Once you've wired the connector to your board:
1. Follow the [Firmware Flash](/docs/getting-started/firmware-flash) guide
2. Configure your board and vehicle in [Configuration](/docs/getting-started/configuration)
3. Test with the Serial Monitor at 115200 baud
