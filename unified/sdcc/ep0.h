// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>

#ifndef EP0_H
#define EP0_H

#include <ezusb_reg.h>

void doSETUP();
void ReEnumberate();
void doSuspend();
void initEP0();

#endif
