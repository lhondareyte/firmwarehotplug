// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>
// Copyright (c) Arnim Laeuger <arniml@users.sourceforge.net>

#ifndef EZUSB_REG_H
#define EZUSB_REG_H
#define AT(X) __at X
#define code __code
#define xdata __xdata
#define sbit __sbit
#define sfr __sfr
#define at __at
#define bit __bit

typedef unsigned char byte;

sbit  at 0x8c TR0; // TCON.4
sbit  at 0x8e TR1; // TCON.6
sbit  at 0x98 RI_0;// SCON.0
sbit  at 0x99 TI_0;// SCON.1
sbit  at 0xa9 ET0; // IE.1
sbit  at 0xac ES0; // IE.4
sbit  at 0xad ET2; // IE.5
sbit  at 0xae ES1; // IE.6
sbit  at 0xaf EA;  // IE.7
sbit  at 0xc0 RI_1;// SCON0.0
sbit  at 0xc1 TI_1;// SCON1.1
sbit  at 0xca TR2; // T2CON.2
sbit  at 0xcf TF2; // T2CON.7
sbit  at 0xe8 EUSB;// EIE.0

sfr   at 0x87 PCON;
sfr   at 0x89 TMOD;
sfr   at 0x8a TL0;
sfr   at 0x8b TL1;
sfr   at 0x8c TH0;
sfr   at 0x8d TH1;
sfr   at 0x91 EXIF;
sfr   at 0xe8 EIE;
sfr   at 0x98 SCON0;
sfr   at 0xc0 SCON1;
sfr   at 0x99 SBUF0;
sfr   at 0xc1 SBUF1;
sfr   at 0x8e CKCON;
sfr   at 0xc8 T2CON;
sfr   at 0xca RCAP2L;
sfr   at 0xcb RCAP2H;
sfr   at 0xcc TL2;
sfr   at 0xcd TH2;
sfr   at 0xd8 EICON;

struct EPCSBC
{
  byte CS;
  byte BC;
};
xdata at 0x7fb4 struct EPCSBC EPx[];
//xdata at 0x7fc4 byte OUT0CS;  //FIXME: marked as "reserved"
//xdata at 0x7fb4 byte EP0CS;

struct USBRequest
{
  byte bmRequest; // 0 - 8
  byte bRequest;  // 1 - 9
  byte wValueL;   // 2 - a
  byte wValueH;   // 3 - b
  byte wIndexL;   // 4 - c
  byte wIndexH;   // 5 - d
  byte wLengthL;  // 6 - e
  byte wLengthH;  // 7 - f
};


#define STALL 0x01 // EPxCS.0
#define HSNAK 0x02 // EPxCS.1
//xdata at 0x7d00 byte IN4BUF;
//xdata at 0x7dc0 byte OUT2BUF;
//xdata at 0x7e00 byte IN2BUF;
xdata at 0x7e40 byte OUT1BUF[64];
xdata at 0x7e80 byte IN1BUF[64];
xdata at 0x7ec0 byte OUT0BUF;
xdata at 0x7f00 byte IN0BUF[64];
xdata at 0x7f92 byte CPUCS;
xdata at 0x7f93 byte PORTACFG;
xdata at 0x7f94 byte PORTBCFG;
xdata at 0x7f95 byte PORTCCFG;
xdata at 0x7f97 byte OUTB;
xdata at 0x7f98 byte OUTC;
xdata at 0x7f9a byte PINSB;
xdata at 0x7f9b byte PINSC;
xdata at 0x7f9d byte OEB;
xdata at 0x7f9e byte OEC;
xdata at 0x7f9f byte UART230;
xdata at 0x7fa1 byte ISOCTL;
xdata at 0x7fa5 byte I2CS;
xdata at 0x7fa6 byte I2DAT;
xdata at 0x7fa9 byte IN07IRQ;
xdata at 0x7faa byte OUT07IRQ;
xdata at 0x7fab byte USBIRQ;
xdata at 0x7fac byte IN07IEN;
xdata at 0x7fad byte OUT07IEN;
xdata at 0x7fae byte USBIEN;
xdata at 0x7faf byte USBBAV;
xdata at 0x7fb4 byte EP0CS;
xdata at 0x7fb5 byte IN0BC;
xdata at 0x7fb6 byte IN1CS;
xdata at 0x7fb7 byte IN1BC;
//xdata at 0x7fb8 byte IN2CS;
//xdata at 0x7fb9 byte IN2BC;
//xdata at 0x7fbc byte IN4CS;
//xdata at 0x7fbd byte IN4BC;
xdata at 0x7fc5 byte OUT0BC;
xdata at 0x7fc6 byte OUT1CS;
xdata at 0x7fc7 byte OUT1BC;
//xdata at 0x7fc8 byte OUT2CS;
//xdata at 0x7fc9 byte OUT2BC;
xdata at 0x7fd4 byte SUDPTRH;
xdata at 0x7fd5 byte SUDPTRL;
xdata at 0x7fd6 byte USBCS;
xdata at 0x7fd8 byte USBFRAMEL;
xdata at 0x7fd9 byte USBFRAMEH;
xdata at 0x7fdd byte USBPAIR;
xdata at 0x7fde byte IN07VAL;
xdata at 0x7fdf byte OUT07VAL;
xdata at 0x7fe3 byte AUTOPTRH;
xdata at 0x7fe4 byte AUTOPTRL;
xdata at 0x7fe5 byte AUTODATA;
xdata at 0x7fe8 struct USBRequest SETUPDAT;

#endif
