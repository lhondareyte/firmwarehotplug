// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>
#include <ezusb_reg.h>
#include <bufsync.h>
#include <uart.h>

// New material for the STC16C552 (2*UART+PORT)

struct St16Ctrl
{
  byte rhr_thr;  // aka SBUF
  byte ier;      // [00]
  byte fcr_isr;  // [00/01]
  byte lcr;      // [00]
  byte mcr;      // [00]
  byte lsr;      // [60]
  byte msr;      // [X0]
  byte spr;      // [ff]
};

union St16Regs
{
  struct St16Ctrl ctrl;
  unsigned int    timer; // Divisor: Rate = CLK / 16 / timer
};

static xdata at 0x7000 union St16Regs portA;
static xdata at 0xb000 union St16Regs portB;

void initSt16(xdata union St16Regs *port)
{
  // init timer
  port->ctrl.lcr = 0x80; // divisor latch enable
  port->timer    = 12;   // = CLK / 500000, where CLK = 6MHz = 24MHz CPU / 4.
  port->ctrl.lcr = 0x03; // select general regs, no parity, 1 stop bit, 8 bits
  // enable interrupts
  port->ctrl.ier = 0x06; // Enable TX/RX interrupts
  port->ctrl.mcr = 0x08; // enable INT A/B
  // fcr, lsr, msr, spr need not be configured (see defaults).
}

// /////////////////////////////////////////////////////////
//
// UART
//
// /////////////////////////////////////////////////////////

byte getPortData(byte port)
{
  switch (port)
  {
    case 0 : return SBUF0;
    case 1 : return SBUF1;
    case 2 : return portA.ctrl.rhr_thr;
    case 3 : return portB.ctrl.rhr_thr;
  }
  return 0; //BUG
}

void putPortData(byte port, byte dta)
{
  switch (port)
  {
    case 0 : SBUF0 = dta; break;
    case 1 : SBUF1 = dta; break;
    case 2 : portA.ctrl.rhr_thr = dta; break;
    case 3 : portB.ctrl.rhr_thr = dta; break;
  }
}

extern void isrUartBottom(byte port); //FIXME: TESTING

void isrUart0() interrupt 4 using 2
{
  if (TI_0) { TI_0 = 0; uart[0].ti = 1; }
  if (RI_0) { RI_0 = 0; uart[0].ri = 1; }
  isrUartBottom(0);
}

void isrUart1() interrupt 7 using 2
{
  if (TI_1) { TI_1 = 0; uart[1].ti = 1; }
  if (RI_1) { RI_1 = 0; uart[1].ri = 1; }
  isrUartBottom(1);
}

//FIXME: make sure interrupts 10,12 are enabled in ezusbmidi.c

void isrUart2() interrupt 10 using 2 // PortA
{
  switch (portA.ctrl.fcr_isr & 0x0f)
  {
    case 0x02 : uart[2].ti = 1; break; // transmitted
    case 0x04 : uart[2].ri = 1; break; // received
  }
  isrUartBottom(2);
}

void isrUart3() interrupt 12 using 2 // PortB
{
  switch (portB.ctrl.fcr_isr & 0x0f)
  {
    case 0x02 : uart[3].ti = 1; break; // transmitted
    case 0x04 : uart[3].ri = 1; break; // received
  }
  isrUartBottom(3);
}

static void initUart(byte port)
{
  uart[port].ti = 0;
  uart[port].ri = 0;
  switch (port)
  {
    case 0: // init UART0 (mode X) baud rate by timer 1 (mode 1)
      ES0      = 0;    // disable serial port 0 interrupt
      SCON0    = 0x50; // 0101 0000
      RI_0     = 0;
      TI_0     = 0;
      ES0      = 1;    // enable serial port 0 interrupt
    break;
    case 1: // init UART1 (mode X) baud rate by timer 1 (mode 1)
      ES1      = 0;    // disable serial port 1 interrupt
      SCON1    = 0x50; // 0101 0000
      RI_1     = 0;
      TI_1     = 0;
      ES1      = 1;    // enable serial port 1 interrupt
    break;
    case 2: // init UART2
      initSt16(&portA);
    break;
    case 3: // init UART3
      initSt16(&portB);
    break;
  }
}

void initSerial()
{ byte i;

  TMOD     = (TMOD & 0x0f) | 0x20;
  TH1      = 0xfe;
  TL1      = 0xfe;
  TR1      = 1;

  for (i = 0; i < 2; i++) initUart(i);
  //FIXME: init upto device->ports in ezusbmidi.c
  //initUart(2);
  //initUart(3);
}
