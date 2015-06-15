# Linux driver for AMB δ1 relay-based R-2R stereo attenuator

The [δ1](http://www.amb.org/audio/delta1/) is an I2C-connected stereo attenuator
intended for audio use. This repository contains a Linux driver for the device
that exposes volume set/query via a sysfs interface.

## sysfs interface

```
# cat /sys/devices/amb_delta1/volume
200
# echo 210 > /sys/devices/amb_delta1/volume
# cat /sys/devices/amb_delta1/volume
210
#
```
