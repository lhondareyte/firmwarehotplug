// [ezusbmidi.c] USB-MIDI adaptor firmware (MAIN)
// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>
/*
   A simple USB-MIDI adaptor firmware for the EZ-USB 2131 chip.
   Copyright (c) 2001 by Lars Doelle <lars.doelle@on-line.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <ezusb_reg.h>
#include <bufsync.h>
#include <uart.h>
#include <ep0.h>
#include <pipeline.h>
#include <leds.h>
#include <desc.h>

// LED material ////////////////////////////////////////////////////

xdata byte leds[MAX_LEDS]; //FIXME: where to put this one?

// //////////////////////////////////////////////////////
//
// I2C BUS EEPROM READER
//
// //////////////////////////////////////////////////////

xdata byte eeprom[8];

static void wait_stop() { while( I2CS & 0x40 ); }
static void wait_done() { while( !(I2CS & 0x01)); }

static void getEEProm()
{ byte i;
  for (i = 0; i < 8; i++) eeprom[i] = 0; // prefill eeprom with all 0
  if ((I2CS & 0x18) != 0x08) return;     // no 8 bit addr EEProm present

  wait_stop();
  I2CS  = 0x80;               // START (write current address)
  I2DAT = 0xA0; wait_done();  // I2C_CMD(10,0,0) 1010-000-0 last bit == WRITE
  I2DAT = 0x00; wait_done();  // new current address = 0
  I2CS  = 0x40;               // STOP

  wait_stop();
  I2CS  = 0x80;               // START (sequential read)
  I2DAT = 0xA1; wait_done();  // I2C_CMD(10,0,1) 1010-000-1 last bit == READ
  eeprom[0] = I2DAT;          // dummy read to trigger sending SCL pulses
  for (i = 0; i < 8; i++)
  {
    wait_done();              // wait for result
    if (i==6) I2CS  = 0x02;   // LastRd
    eeprom[i] = I2DAT;        // get result, trigger next if not LastRd
  }
  I2CS = 0x40;                // STOP
}

struct EEPromData
{
  unsigned short vendor;
  unsigned short product;
  unsigned short device;
};

// Pins and Leds ////////////////////////////////////////////////////////////////
//
// The 8051 has a group of registers controling its pins. The content of these
// registers reflects the particular wireing of the device.
//
// The pins?x? below contains the these registers in the following order:
// - PORTxCNF OEx OUTx (x = A,B,C)
// -  1        9        17     24
// - "-------- -------0 -LLL--RT" // Midisport1x1
// - "-------- ----RT-0 -LLLLLRT" // Midisport2x2
// - "LLLLLLLL ----RT-0 ---L--RT" // Midisport4x4
// - "-------- ----RT-0 ------RT" // generic
//                    ^
//                    |
//              makes LEDs operational
//
// L - LEDs           0 1 X
// - - Don't care     0 0 0
// 0 - Special        0 1 0
// R - alternate      1 1 1
// T - alternate      1 1 1
//

static code byte pins1x1[] = { 0x03, 0x73, 0x03,  0x00, 0x01, 0x00 };
static code byte pins2x2[] = { 0x03, 0x7F, 0x03,  0x0C, 0x0D, 0x0C };
static code byte pinsNxN[] = { 0x03, 0x03, 0x03,  0x0C, 0x0D, 0x0C };

//                               c4    c5    c6
static code byte leds1x1[] = { 0x10, 0x20, 0x40 };
//                               c6    c5    c4    c3    c2
static code byte leds2x2[] = { 0x40, 0x20, 0x10, 0x08, 0x04 };
static code byte ledsNxN[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

struct Pins { byte cfg; byte oe; byte out; };

//                                        A cfg   oe    out     B                     C
static code struct Pins new_pins1x1[] = { { 0x00, 0x00, 0x00 }, { 0x00, 0x01, 0x00 }, { 0x03, 0x73, 0x03 } };
static code struct Pins new_pins2x2[] = { { 0x00, 0x00, 0x00 }, { 0x0C, 0x0D, 0x0C }, { 0x03, 0x7F, 0x03 } };
static code struct Pins new_pins4x4[] = { { 0x00, 0x00, 0x00 }, { 0x0C, 0x0D, 0x0C }, { 0x03, 0x7F, 0x03 } };
static code struct Pins new_pinsNxN[] = { { 0x00, 0x00, 0x00 }, { 0x0C, 0x0D, 0x0C }, { 0x03, 0x7F, 0x03 } };

// lednums are Port(a,b,c) and Bit (7-0), e.g. 0xb5 = Port B Bit 5. 0x00 means no LED.
// order is: Usb, { In, Out }

static code byte new_leds1x1[] = { 0xc4,  0xc5, 0xc6 };
static code byte new_leds2x2[] = { 0xc6,  0xc5, 0xc4,  0xc3, 0xc2 };
static code byte new_leds4x4[] = { 0xc4,  0xa0, 0xa1,  0xa2, 0xa3,  0xa4, 0xa5,  0xa6, 0xa7 };
static code byte new_ledsNxN[] = { 0x00,  0x00, 0x00,  0x00, 0x00 };

struct Device
{
  unsigned short vendor;
  unsigned short product;
  byte           ports;
  code char*     name;
  code byte*     leds;
  code byte*     pins;
};

static code struct Device devices[] =
{
  { 0x0763, 0x1010, 1, "Midisport 1x1",    leds1x1, pins2x2 },
  { 0x0763, 0x1001, 2, "Midisport 2x2",    leds2x2, pins2x2 },
  { 0x0763, 0x1020, 4, "Midisport 4x4",    ledsNxN, pins2x2 }, //FIXME: preliminary
//{ 0x0763, 0x1030, 8, "Midisport 8x8",    ledsNxN, pins2x2 }, //FIXME: preliminary
  { 0x0000, 0x0000, 2, "USB MIDI Adaptor", ledsNxN, pins2x2 }
};
#define DEV_CNT (sizeof(devices)/sizeof(struct Device))

static code struct Device* device;

void findDevice()
{ xdata struct EEPromData* eedata = (xdata struct EEPromData*)(eeprom+1);
  byte i;
  for (i = 0; i < DEV_CNT-1; i++)
    if (eedata->vendor  == devices[i].vendor &&
        eedata->product == devices[i].product)
      break;
  device = &devices[i];
}

void initPins()
{
  // init LED ports, direct UART Rx/Tx to pins.
  PORTCCFG  = device->pins[0]; // 0000 0011 - alternate
  OEC       = device->pins[1]; // 0LLL 00RT - L:led, R:rx, T:tx
  OUTC      = device->pins[2]; // 0000 0011 - all leds on, Rx/Tx to pins

  PORTBCFG  = device->pins[3];
  OEB       = device->pins[4]; // xxxx xxx1
  OUTB      = device->pins[5]; // xxxx xxx0

  UART230   = 0x01; // not needed
}

#if 1
static void setLeds()
{ byte i; byte bits;
  ledUSB = (USBFRAMEH>>2) == ( (((USBFRAMEH<<3)|(USBFRAMEL>>5))&0x1f)
                             > (USBFRAMEL&0x1f) );
  if ((USBFRAMEL&0x0f) == 0) for (i = 1; i < 2*device->ports+1; i++) leds[i] = 0;
  bits = 0;
  for (i = 0; i < 2*device->ports+1; i++) bits |= leds[i]?0:device->leds[i];
  OUTC = bits | device->pins[2];
}
#else
static byte code power2[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
static void setLeds()
{ byte i; byte bits[3];
  ledUSB = (USBFRAMEH>>2) == ( (((USBFRAMEH<<3)|(USBFRAMEL>>5))&0x1f)
                             > (USBFRAMEL&0x1f) );
  if ((USBFRAMEL&0x0f) == 0) for (i = 1; i < 2*device->ports+1; i++) leds[i] = 0;
  bits[0] = bits[1] = bits[2] = 0;
  for (i = 0; i < 2*device->ports+1; i++)
  { byte led = device->leds[i];
    byte port = ((led>>4)&0x0f)-0x0a;
    byte bitn = ((led>>0)&0x07);
    if (port > 2) continue;
    bits[port] |= leds[i]?0:power2[bitn];
  }
  OUTA = bits[0] | device->pins[0].out;
  OUTB = bits[1] | device->pins[1].out;
  OUTC = bits[2] | device->pins[2].out;
}
#endif

#if 0
// ////////////////////////////////////////////////////////
//
// Timer 0
//
// We could use the timer for time driven events based
// on non-usb events. We do not really have some, so the
// section is disabled.
//
// ////////////////////////////////////////////////////////


static unsigned int timerTicks;
void initTimer()
{
  // init TIMER 0 (not really needed)

  TMOD     = (TMOD & 0xf0) | 0x00; // timer 0, mode 0
  TH0      = 0;
  TL0      = 0;
  TR0      = 1; // enable timer 0
  ET0      = 1; // enable timer 0 interrupt

  timerTicks = 0; // ca  1/4 ms counter
}

static void isrTime0() interrupt 1 using 1 // 244 Hz
{
  timerTicks += 1;
}
#endif

// ////////////////////////////////////////////////////////
//
// Interrupt Service Routines
//
// ////////////////////////////////////////////////////////

//FIXME: we should properly stall EP0 SETUP
//       if we did not complete processing SETUP

static bit ep1_ri;
static bit ep1_ti;

// other bottom half indicators

static bit bitSUDAVSeen = 0;
static bit bitSUSPSeen  = 0;

static void isrUsb(void) __interrupt(8) __using(3)
{
   EXIF &= 0xef; // clear INT2 interrupt
   if (  USBIRQ & 0x01) {   USBIRQ = 0x01; bitSUDAVSeen = 1; } // Setup  data avail
   if (  USBIRQ & 0x02) {   USBIRQ = 0x02;                   } // Start of Frame
   if (  USBIRQ & 0x04) {   USBIRQ = 0x04;                   } // Setup Token (FIXME: stall if not processed)
   if (  USBIRQ & 0x08) {   USBIRQ = 0x08; bitSUSPSeen  = 1; } // Suspend
   if (  USBIRQ & 0x10) {   USBIRQ = 0x10; ep1_ti       = 1; } // USB Reset
   if (OUT07IRQ & 0x02) { OUT07IRQ = 0x02; ep1_ri       = 1; } // EP 1 OUT
   if ( IN07IRQ & 0x02) {  IN07IRQ = 0x02; ep1_ti       = 1; } // EP 1 IN
}

// ///////////////////////////////////////////////////////////////////////
//
// Initialize 8051 and EZUSB hardware
//
// ///////////////////////////////////////////////////////////////////////

void initUSB()
{
  // init USB core //FIXME: set proper values

  USBPAIR   = 0x00; //

  OUT07VAL  = 0x02; //
  OUT1BC    = 0x00; // Arm EP 1 OUT
  OUT1CS    = 0x00; // Unstall EP 1 OUT
  OUT07IRQ  = 0xff;

  IN07VAL   = 0x02; //
  IN1CS     = 0x00; // Unstall EP 1 IN
  IN07IRQ   = 0xff;

  EUSB      = 1;
  EICON     = 0x20;

  USBIRQ    = 0xff;

  EXIF      = 0x00;

  USBBAV    = 0x00; // deactivated AVEC
  USBIEN    = 0x1d; // 0001 1101
  CPUCS     = 0x02; // CLK24OE
}

// //////////////////////////////////////////////////////
//
// MAIN
//
// //////////////////////////////////////////////////////

byte _sdcc_external_startup()
{
  ISOCTL |= 1; // ISODISAB = 1; // make xdata memory available at 0x2000 .. 0x27ff
  return 0;
}

void Configure()
{
  getEEProm();
  findDevice();

  ledUSB = 0;

  initPipes(device->ports);
  initEP0();
  makeDesc(device->ports, device->name, eeprom+1);
  initPins  ();
  initSerial();
  initUSB   ();

  EA = 1;
}

void main() // Never Terminates
{

  Configure();
  ReEnumberate();

  // Main Event Loop

  for (;;)
  { //byte i;
    // Bottom Half ISRs
    if (bitSUDAVSeen) { bitSUDAVSeen = 0; doSETUP();   } // Handle SUDAV Events
    if (bitSUSPSeen)  { bitSUSPSeen  = 0; doSuspend(); } // Handle SUSP Events
    if (ep1_ri)       { ep1_ri = 0;       doEP1_ri();  } // EP 1 OUT buffer clear
    if (ep1_ti)       { ep1_ti = 0;       doEP1_ti();  } // EP 1 IN  buffer clear

    //FIXME: pipeline operation should be suspended until config
//  for (i = 0; i < device->ports; i++) isrUartBottom(i);
    runPipes(device->ports);                              // run UART <-> USB
    setLeds();                                            // refresh Leds
  }
}
