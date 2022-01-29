# M-AUDIO MIDISPORT opensource firmwares
This repository is fork of the [firmwarehotplug project](https://sourceforge.net/projects/linux-hotplug/files/) with patches to compile with the latest version of [SDCC compiler](http://sdcc.sourceforge.net). See README file for credits and authors.

## What for?
Official M-AUDIO firmwares are available on most Linux distros. Unfortunately, it does not work with the ```fxload``` version of FreeBSD. This open source version solve the problem.

## Which version?
There are two flavours of firmware: original and unified. Original version contains firmwares for Midiman 1x1 and 2x2 interfaces. The unified version contains one unique firmware that is known to support the following interfaces:
```
* MidiSportUNO (Midiman) : operational (= 1x1)
* MidiSport1x1 (Midiman) : operational
* MidiSport2x2 (Midiman) : operational
* USB-2-MIDI (Steinberg) : operational (= 2x2)
* MidiSport4x4 (Midiman) : first two ports operational, no leds, work in progress
* MidiSport8x8 (Midiman) : unknown, perhaps like 4x4
```
Original version is recommended by the authors.

## Installation
First, choose unified or original version:
```
cd original
sudo gmake
sudo gmake freebsd-install
sudo service devd restart
```
Plug your USB interface, you should have one device per MIDI port available:
```
ls -l /dev/umidi*
crw-r--r--  1 root  operator  0x6d Jul 29 12:15 /dev/umidi0.0
crw-r--r--  1 root  operator  0x6e Jul 29 12:15 /dev/umidi0.1

## Port
For convenience, a port [is available here](https://github.com/lhondareyte/ports/tree/master/firmwarehotplug).
```
