# Linux driver for AMB δ1 relay-based R-2R stereo attenuator

The [δ1](http://www.amb.org/audio/delta1/) is an I2C-connected stereo attenuator
intended for audio use. This repository contains a Linux driver for the device
that exposes volume set/query via a sysfs interface.

This was written primarily for use with a Raspberry Pi, but with a modified
devicetree overlay, should work fine on other devices as well.

## sysfs interface

```
# cat /sys/devices/amb_delta1/volume
200
# echo 210 > /sys/devices/amb_delta1/volume
# cat /sys/devices/amb_delta1/volume
210
#
```

## Interfacing with a Raspberry Pi

The included `amb-delta1-rpi-overlay.dts` is a devicetree overlay intended for
use on a Raspberry Pi with a δ1 directly interfaced to its i2c bus.

Note, however, that while the δ1 was designed with a 5V i2c bus in mind, the
Raspberry Pi's i2c bus is 3.3V. Therefore, there are two options:

   - Add level shifters between the RPI's i2c bus and the δ1. There are numerous
     ways to do this, but I opted instead to:
   - Modify the δ1 to work on a 3.3V i2c bus

The modifications are pretty straight forward. The PCF8574As that sit on the
δ1's i2c bus have no issues operating at 3.3V, and the board's provision for
providing an alternative power supply for driving the relays makes this pretty
easy:

   - Replace R5+/R5- with 27KΩx8 bussed array
   - Install a jumper across JP2 pins 2-3 to power the relays with "ALT PWR IN"
   - Connect a 5V power supply to "ALT PWR IN" (J1 pin 2) for the relays
   - Connect a 3.3V power supply to "+5V IN" (J1 pin 1) for U1+/U1-. You can
     pull this directly from the RPi's expansion header.
