# waveshare_poe_b

## Thermal Cooling kernel module

This is a kernel driver for exposing the [Waveshare Raspberry Pi PoE Hat (B)](https://www.waveshare.com/wiki/PoE_HAT_(B)) as a thermal cooling device under Linux so you can add thermal CPU triggers to enable/disable the fan.

This is a WIP and has currently only been loaded by hand with `insmod` on a running kernel. Loading/dependencies TBD.

## Requirements

* A Raspberry Pi 4
* A Waveshare PoE Hat (B)
* A Linux kernel with existing I2C support 

This module doesn't actually implement I2C over GPIO directly and instead requires that your kernel already supports I2C on the Raspberry Pi. I'm pretty that includes both Raspbian and Ubuntu; I've only tested it on the official Ubuntu 20.04 release for Raspberry Pi.

## Compiling

1. Clone this repo to your Raspberry Pi.

2. Make sure the kernel headers are installed:

   `apt-get install linux-headers-raspi`
   
3. Run `make`

## Usage

#### Load the module with
```bash
insmod waveshare_poe_b.ko
```

#### Change the fan settings with:
Assuming you don't have other cooling devices already...

**NB** Also don't forget to toggle the physical switch to `P0` to enable software control.

```bash
# Turn on the fan
echo 1 > /sys/class/thermal/cooling_device0/cur_state

# Turn off the fan
echo 0 > /sys/class/thermal/cooling_device0/cur_state

#### Unload the module with
```bash
rmmod waveshare_poe_b.ko
```

**TODO**: Document Linux fan trigger config

## Future work
Next up, OLED support.