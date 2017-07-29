// [desc.h] Generate multiport descriptors
// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>

#ifndef DESC_H
#define DESC_H

#include <ezusb_reg.h>

void makeDesc(byte ports, code char* name, xdata byte* ids);
//xdata byte* findDescriptor(byte key, byte index);
extern xdata byte Desc[]; //FIXME: only for findDescriptor

#endif
