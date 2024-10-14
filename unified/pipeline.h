// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>
#ifndef PIPELINE_H
#define PIPELINE_H

#include <ezusb_reg.h>

void isrUartBottom(byte port);
void doEP1_ri(void);
void doEP1_ti(void);

void initPipes(byte ports);
void runPipes(byte ports);

#endif
