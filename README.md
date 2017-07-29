## M-AUDIO MIDISPORT opensource firmwares

This repository is fork of the [firmwarehotplug project](https://sourceforge.net/projects/linux-hotplug/files/) with patches to compile with latest version of sdcc compiler. See README file for credits and authors.


## Notes 
Official M-AUDIO firmwares are available on most Linux distros. Unfortunately, it does not work with the fxload version of FreeBSD. This open source version solve the problem:

To install: 
Choose unified or original version (original version is recommended).
```
cd original
sudo gmake
sudo gmake freebsd-install
```
Plug your USB interface, you should have MIDI devices availables:
```
ls -l /dev/umidi*
crw-r--r--  1 root  operator  0x6d Jul 29 12:15 /dev/umidi0.0
crw-r--r--  1 root  operator  0x6e Jul 29 12:15 /dev/umidi0.1
```
