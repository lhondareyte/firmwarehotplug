// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>
#ifndef UART_H
#define UART_H

#include <ezusb_reg.h>
#include <ezusbmidi.h>
#define AT(X) __at X
#define code __code
#define xdata __xdata
#define sbit __sbit
#define sfr __sfr
#define at __at
#define interrupt __interrupt
#define using __using

struct Uart
{
  byte ti;
  byte ri;
};

extern struct Uart uart[MAX_PORTS];

byte getPortData(byte port);
void putPortData(byte port, byte dta);
void initSerial();

void isrUart0() interrupt  4 using 2;
void isrUart1() interrupt  7 using 2;
void isrUart2() interrupt 10 using 2; // 4x4 PortA
void isrUart3() interrupt 12 using 2; // 4x4 PortB

#endif
