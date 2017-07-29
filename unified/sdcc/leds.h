// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>
#ifndef LEDS_H
#define LEDS_H

#include <ezusbmidi.h>

extern xdata byte leds[MAX_LEDS];
#define ledUSB    leds[0]
#define ledIN(X)  leds[1+2*(X)]
#define ledOUT(X) leds[2+2*(X)]

#endif
