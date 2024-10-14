// [ezusbmidi.c]
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

/* TODO:
   - Make both channels in MidiSport2x2 run together
*/

#include <ezusb_reg.h>
#include <bufsync.h>

#undef HAS_TIMER_0
#define RAW_MIDI 0

#if defined(CONFIG_MidiSport1x1) == defined(CONFIG_MidiSport2x2)
#error "either CONFIG_MidiSport1x1 or CONFIG_MidiSport2x2 has to be set"
#endif

// Static Variables (initialized) //////////////////////////////////

static bit bitSUDAVSeen = 0;
static bit bitSUSPSeen  = 0;

static bit ledIN  = 0;
static bit ledOUT = 0;
static bit ledUSB = 0;
#ifdef CONFIG_MidiSport2x2
static bit ledIN1  = 0;
static bit ledOUT1 = 0;
#endif

#ifdef HAS_TIMER_0
static volatile data byte timerTicksL = 0x00; // ca  1/2 ms counter
static volatile data byte timerTicksH = 0x00; // ca  1 sec counter
#endif

// Here come the variable for the Petri net. /////////////////////////

// USB EP (in/out)

static volatile data  struct LINSYN  inusbCtl;        // EP 1 IN control
static xdata at 0x7e80 byte          inusbBuf[0x40];  // IN1BUF

// EP 1 OUT -> UART (0) IN pipeline

static volatile data  struct LINSYN  outusbCtl;       // EP 1 OUT control
static xdata at 0x7e40 byte          outusbBuf[0x40]; // OUT1BUF

// UART (0) -> EP 1 pipeline

static volatile data  struct CYCSYN  inserCtl;        // UART->serial
static xdata  at 0x7b80 byte         inserBuf[0x40];  // IN7BUF

static volatile data  struct CYCSYN  outserCtl;       // serial->UART
static xdata at 0x7b40 byte          outserBuf[0x40]; // OUT7BUF, tmp

static volatile data  struct LINSYN  outuarCtl;       // -> UART
//                                   SBUF0

#ifdef CONFIG_MidiSport2x2
// UART (1) -> EP 1 pipeline

static volatile data  struct CYCSYN  inserCtl1;        // UART->serial
static xdata  at 0x7c00 byte         inserBuf1[0x40];  // IN7BUF

// EP 1 OUT -> UART (1) IN pipeline

static volatile data  struct CYCSYN  outserCtl1;       // serial->UART
static xdata at 0x7c80 byte          outserBuf1[0x40]; // OUT7BUF, tmp

static volatile data  struct LINSYN  outuarCtl1;       // -> UART
#endif

static data byte configuration; // 1 if in configured state


// ///////////////////////////////////////////////////////////////////////
//
// Initialize 8051 and EZUSB hardware
//
// ///////////////////////////////////////////////////////////////////////

void initUSB(void)
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

void initPorts(void)
{
  // init LED ports, direct UART Rx/Tx to pins.

#ifdef CONFIG_MidiSport1x1
  PORTCCFG  = 0x03; // 0000 0011 - alternate
  OEC       = 0x73; // 0LLL 00RT - L:led, R:rx, T:tx
  OUTC      = 0x03; // 0000 0011 - all leds on, Rx/Tx to pins
#else /* MidiSport2x2 */
  PORTCCFG  = 0x03; // 0000 0011 - alternate
  OEC       = 0xFF; // LLLL LLRT - L:led, R:rx, T:tx
  OUTC      = 0x03; // 0000 0011 - all leds on, Rx/Tx to pins
#endif

  // Set PB0 to 0. (FIXME: for what?)
#ifdef CONFIG_MidiSport1x1
  PORTBCFG  = 0x00;
  OEB       = 0x01; // xxxx xxx1
  OUTB      = 0x00; // xxxx xxx0
#else
  PORTBCFG  = 0x0C; // 0000 1100 - alternate
  OEB       = 0x0D; // 0000 RT0? - R:rx, T:tx of UART1 
  OUTB      = 0x0C; // 0000 1100 - Rx/Tx to pins
#endif

  UART230   = 0x01; // not needed
}

#ifdef HAS_TIMER_0
void initTimer()
{
  // init TIMER 0 (not really needed)

  TMOD     = (TMOD & 0xf0) | 0x00; // timer 0, mode 0
  TH0      = 0;
  TL0      = 0;
  TR0      = 1; // enable timer 0
  ET0      = 1; // enable timer 0 interrupt
}
#endif

void initSerial(void)
{
  // init UART0 (mode X) baud rate by timer 1 (mode X)

  TMOD     = (TMOD & 0x0f) | 0x20;
  TH1      = 0xfe;
  TL1      = 0xfe;
  TR1      = 1;

  ES0      = 0;    // disable serial port 0 interrupt
  SCON0    = 0x50; // 0101 0000
  RI_0     = 0;
  TI_0     = 0;
  ES0      = 1;    // enable serial port 0 interrupt

#ifdef CONFIG_MidiSport2x2
  // init UART1 (mode X) baud rate by timer 1 (mode X)
  ES1      = 0;    // disable serial port 1 interrupt
  SCON1    = 0x50; // 0101 0000
  RI_1     = 0;
  TI_1     = 0;
  ES1      = 1;    // enable serial port 1 interrupt
#endif
}

static void initPipes(void)
{
  CYCINIT(inserCtl);
  LININIT(inusbCtl);

  LININIT(outusbCtl);
  CYCINIT(outserCtl);
  LININIT(outuarCtl);

#ifdef CONFIG_MidiSport2x2
  CYCINIT(inserCtl1);
  CYCINIT(outserCtl1);
  LININIT(outuarCtl1);
#endif
}

// ////////////////////////////////////////////////////////
//
// Interrupt Service Routines
//
// ////////////////////////////////////////////////////////
// FIXME: check register banks (using)

static void isrWakeup(void) //interrupt FIXME: reactivate
{
  EICON &= 0xef;
}

#if HAS_TIMER_0
static void isrTime0(void) __interrupt(1) __using(1) //critical // 244 Hz
{
  if (++timerTicksL == 0) ++timerTicksH;
}
#endif

static void isrUart0(void) __interrupt(4) __using(1) //critical
{
  if (TI_0)
  {
    TI_0 = 0;
    ledOUT ^= 1;
    if (LINCANREAD(outuarCtl)) LINRDRDONE(outuarCtl);
  }
  if (RI_0)
  { byte dta = SBUF0;
    RI_0 = 0;
    if (dta!=0xfe) ledIN ^= 1;      // ignore active-sensing
    if (!BUFFULL(inserCtl)) inserBuf[BUFPUT(inserCtl)] = dta;
  }
}

#ifdef CONFIG_MidiSport2x2
static void isrUart1(void) __interrupt(7) __using(2) //critical
{
  if (TI_1)
  {
    TI_1 = 0;
    ledOUT1 ^= 1;
    if (LINCANREAD(outuarCtl1)) LINRDRDONE(outuarCtl1);
  }
  if (RI_1)
  { byte dta = SBUF1;
    RI_1 = 0;
    if (dta!=0xfe) ledIN1 ^= 1;     // ignore active-sensing
    if (!BUFFULL(inserCtl1)) inserBuf1[BUFPUT(inserCtl1)] = dta;
  }
}
#endif

//FIXME: we should properly stall EP0 SETUP
//       if we did not complete processing SETUP

static void isrUsb(void) __interrupt(8) __using(3) //critical
{
   EXIF &= 0xef; // clear INT2 interrupt
   if (  USBIRQ & 0x01) {   USBIRQ = 0x01; bitSUDAVSeen = 1;       } // Setup Data avail
   if (  USBIRQ & 0x02) {   USBIRQ = 0x02;                         } // Start of Frame
   if (  USBIRQ & 0x04) {   USBIRQ = 0x04;                         } // Setup Token
   if (  USBIRQ & 0x08) {   USBIRQ = 0x08; bitSUSPSeen  = 1;       } // Suspend
   if (  USBIRQ & 0x10) {   USBIRQ = 0x10; LINRDRDONE(inusbCtl);   } // USB Bus Reset
   if ( IN07IRQ & 0x02) {  IN07IRQ = 0x02; LINRDRDONE(inusbCtl);   } // IN  Bulk transfered
   if (OUT07IRQ & 0x02) { OUT07IRQ = 0x02; outusbCtl.wrt = OUT1BC;   // OUT Bulk transfered
                                           LINWRTDONE(outusbCtl);  }
}

// ///////////////////////////////////////////////////////////////////////
//
// EP 0 Control Protocol
//
// ///////////////////////////////////////////////////////////////////////

static code byte Descriptors[] =
{
//FIXME check bmAttributes & MaxPower
  0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x40, 0x63, // Device ...
   0x07,0x10, 0x11, 0x01, 0x00, 0x01, 0x02, 0x03, 0x01, //  ...
#ifdef CONFIG_MidiSport1x1
  0x09, 0x02, 0x65, 0x00, 0x02, 0x01, 0x00, 0x00, 0x32, // Config
#else
  0x09, 0x02, 0x85, 0x00, 0x02, 0x01, 0x00, 0x00, 0x32, // Config
#endif
  0x09, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, // Interface 0
  0x09, 0x24, 0x01, 0x00, 0x01, 0x09, 0x00, 0x01, 0x01, // CS Interface (audio)
  0x09, 0x04, 0x01, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00, // Interface 1
#ifdef CONFIG_MidiSport1x1
  0x07, 0x24, 0x01, 0x00, 0x01, 0x41, 0x00,             // CS Interface (midi)
  0x06, 0x24, 0x02, 0x01, 0x01, 0x00,                   //   IN  Jack 1 (emb)
  0x06, 0x24, 0x02, 0x02, 0x02, 0x00,                   //   IN  Jack 2 (ext)
  0x09, 0x24, 0x03, 0x01, 0x03, 0x01, 0x02, 0x01, 0x00, //   OUT Jack 3 (emb)
  0x09, 0x24, 0x03, 0x02, 0x04, 0x01, 0x01, 0x01, 0x00, //   OUT Jack 4 (ext)
  0x09, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, // Endpoint OUT
  0x05, 0x25, 0x01, 0x01, 0x01,                         //   CS EP IN  Jack
  0x09, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, // Endpoint IN
  0x05, 0x25, 0x01, 0x01, 0x03,                         //   CS EP OUT Jack
#else
  0x07, 0x24, 0x01, 0x00, 0x01, 0x61, 0x00,             // CS Interface (midi)
  0x06, 0x24, 0x02, 0x01, 0x01, 0x00,                   //   IN  Jack 1 (emb)
  0x06, 0x24, 0x02, 0x02, 0x02, 0x00,                   //   IN  Jack 2 (ext)
  0x06, 0x24, 0x02, 0x01, 0x03, 0x00,                   //   IN  Jack 3 (emb)
  0x06, 0x24, 0x02, 0x02, 0x04, 0x00,                   //   IN  Jack 4 (ext)
  0x09, 0x24, 0x03, 0x01, 0x05, 0x01, 0x02, 0x01, 0x00, //   OUT Jack 5 (emb)
  0x09, 0x24, 0x03, 0x02, 0x06, 0x01, 0x01, 0x01, 0x00, //   OUT Jack 6 (ext)
  0x09, 0x24, 0x03, 0x01, 0x07, 0x01, 0x02, 0x01, 0x00, //   OUT Jack 7 (emb)
  0x09, 0x24, 0x03, 0x02, 0x08, 0x01, 0x01, 0x01, 0x00, //   OUT Jack 8 (ext)
  0x09, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, // Endpoint OUT
  0x06, 0x25, 0x01, 0x02, 0x01, 0x03,                   //   CS EP IN  Jack
  0x09, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, // Endpoint IN
  0x06, 0x25, 0x01, 0x02, 0x05, 0x07,                   //   CS EP OUT Jack
#endif
  0x04, 0x03, 0x09, 0x04,                               // String Lang:EN
  0x10, 0x03, // Midiman                                // String iManufacturer
  'M',0, 'i',0, 'd',0, 'i',0, 'm',0, 'a',0, 'n',0,
#ifdef CONFIG_MidiSport1x1
  0x1c, 0x03, // MidiSport 1x1                          // String iProduct
  'M',0, 'i',0, 'd',0, 'i',0, 's',0, 'p',0, 'o',0,
  'r',0, 't',0, ' ',0, '1',0, 'x',0, '1',0,
#endif
#ifdef CONFIG_MidiSport2x2
  0x1c, 0x03, // MidiSport 2x2                          // String iProduct
  'M',0, 'i',0, 'd',0, 'i',0, 's',0, 'p',0, 'o',0,
  'r',0, 't',0, ' ',0, '2',0, 'x',0, '2',0,
#endif
  // Copyright (GPLv2) 2001 by Lars Doelle <lars.doelle@on-line.de>
  0x7e, 0x03,                                           // String iSerialnumber
  'C',0, 'o',0, 'p',0, 'y',0, 'r',0, 'i',0, 'g',0,
  'h',0, 't',0, ' ',0, '(',0, 'G',0, 'P',0, 'L',0,
  'v',0, '2',0, ')',0, ' ',0, '2',0, '0',0, '0',0,
  '1',0, ' ',0, 'b',0, 'y',0, ' ',0, 'L',0, 'a',0,
  'r',0, 's',0, ' ',0, 'D',0, 'o',0, 'e',0, 'l',0,
  'l',0, 'e',0, ' ',0, '<',0, 'l',0, 'a',0, 'r',0,
  's',0, '.',0, 'd',0, 'o',0, 'e',0, 'l',0, 'l',0,
  'e',0, '@',0, 'o',0, 'n',0, '-',0, 'l',0, 'i',0,
  'n',0, 'e',0, '.',0, 'd',0, 'e',0, '>',0,
  0x00
};

#define IsSelfPowerDevice 0
static bit bitRemoteWakeup = 0; // deactivated at reset

#define EP2EP(EP) ((((EP)>>4)&0x08)|(EP)&0x07)
static void ctrlGetStatus(void)
{
  switch (SETUPDAT.bmRequest)
  {
    case 0x80: // IN, Device (Remote Wakeup and Self Powered Bit)
               IN0BUF[0] = (bitRemoteWakeup << 1) | IsSelfPowerDevice;
               IN0BUF[1] = 0x00;
               IN0BC     = 0x02;
               EP0CS     = HSNAK; // Acknowledge Control transfer
               break;
    case 0x81: // IN, Configuration (reserved)
               IN0BUF[0] = 0x00; // (reserved, set to zero)
               IN0BUF[1] = 0x00; // (reserved, set to zero)
               IN0BC     = 0x02;
               EP0CS     = HSNAK; // Acknowledge Control transfer
               break;
    case 0x82: // IN, Endpoint (Stall Bits)
               IN0BUF[0] = EPx[EP2EP(SETUPDAT.wIndexL)].CS & 0xfd;//&~EP_BUSY
               IN0BUF[1] = 0x00;
               IN0BC     = 0x02;
               EP0CS     = HSNAK; // Acknowledge Control transfer
               break;
    default:   EP0CS = STALL; // STALL indicating Request Error
               break;
  }
}

// FIXME: this section certainly needs some cleanups
// - STALL/BC/HSNAK cleanups
// - Behaviour cleanups

static void ctrlGetConfiguration(void)
{
  IN0BUF[0] = configuration;
  IN0BC = 0x01;
  EP0CS = HSNAK; // Acknowledge Control transfer
}

static void ctrlGetInterface(void)
{
  //FIXME: issue stall if not in configured state
  IN0BUF[0] = 0;
  IN0BC = 0x01;
  EP0CS = HSNAK; // Acknowledge Control transfer
}

static void ctrlSetConfiguration(void)
{
  //FIXME: issue stall if configuration not in {0,1}
  configuration = SETUPDAT.wValueL&1;
//FIXME: (re)set thing accordingly
  EP0CS = HSNAK; // Acknowledge Control transfer
}

static void ctrlSetInterface(void)
{
  //FIXME: issue stall on invalid arguments
//       0 == SETUPDAT.wIndexL; // interface
//interface = SETUPDAT.wValueL; // al
//FIXME: (re)set thing accordingly
  EP0CS = HSNAK; // Acknowledge Control transfer
}

static void ctrlClearFeature(void)
{
  switch (SETUPDAT.bmRequest)
  {
    case 0x00: // Device Feature (Remote Wakeup)
               if (SETUPDAT.wValueL == 0x01)
               {
                 bitRemoteWakeup = 0;
                 EP0CS = HSNAK; // Acknowledge Control transfer
               }
               else
                 EP0CS = STALL;
               break;
    case 0x02: // Endpoint Feature (Stall Endpoint)
               if (SETUPDAT.wValueL == 0x00)
               {
                 EPx[EP2EP(SETUPDAT.wIndexL)].CS = 0x00;
                 EP0CS = HSNAK; // Acknowledge Control transfer
               }
               else
                 EP0CS = STALL;
               break;
    default: EP0CS = STALL; break;
  }
}

static void ctrlSetFeature(void)
{
  switch (SETUPDAT.bmRequest) // SETUPDAT.bmRequest
  {
    case 0x00: // Device Feature (Remote Wakeup)
               if (SETUPDAT.wValueL == 0x01)
               {
                 bitRemoteWakeup = 1;
                 EP0CS = HSNAK; // Acknowledge Control transfer
               }
               else
                 EP0CS = STALL;
               break;
    case 0x02: // Endpoint Feature (Stall Endpoint)
               if (SETUPDAT.wValueL == 0x00)
               {
                 EPx[EP2EP(SETUPDAT.wIndexL)].CS = 0x01;
                 EP0CS = HSNAK; // Acknowledge Control transfer
               }
               else
                 EP0CS = STALL;
               break;
    default: EP0CS = STALL; break;
  }
}

static void ctrlGetDescriptor(void)
{
  unsigned int p;
  byte key   = SETUPDAT.wValueH;
  byte index = SETUPDAT.wValueL;
  byte count = 0;

  for (p = 0; Descriptors[p]; p+=Descriptors[p])
  if (Descriptors[p+1] == key && count++ == index)
  {
    SUDPTRH = (byte)((((unsigned int)&Descriptors[p]))>>8)&0xff;
    SUDPTRL = (byte)(  (unsigned int)&Descriptors[p]     )&0xff;
//  FIXME: something is weird, here
//         may be we got asked things, we cannot answer
//  EP0CS = HSNAK; // Acknowledge Control transfer
//  return;
  }
  EP0CS = HSNAK; // Acknowledge Control transfer
//EP0CS = STALL; // Acknowledge Control transfer
}

static void doSETUP(void)
{
  switch  (SETUPDAT.bRequest)
  {
    case 0x00: ctrlGetStatus();         break;
    case 0x01: ctrlClearFeature();      break;
//  case 0x02: /*  reserved */          break; // unsupported
    case 0x03: ctrlSetFeature();        break;
//  case 0x04: /* reserved */           break; // unsupported
    case 0x05: /*  SetAddress */        break; //FIXME: (test) handled by EZUSB core
    case 0x06: ctrlGetDescriptor();     break;
//  case 0x07: /*  SetDescriptor */     break; // unsupported
    case 0x08: ctrlGetConfiguration();  break;
    case 0x09: ctrlSetConfiguration();  break;
    case 0x0a: ctrlGetInterface();      break;
    case 0x0b: ctrlSetInterface();      break;
    case 0x0c: /* Sync: SOF */          break; //FIXME: (test) handled by EZUSB core
    default  : EP0CS = STALL;           break;
  }
  bitSUDAVSeen = 0;
  USBIEN |= 0x02;
}

// ////////////////////////////////////////////////////////////////////////
//
// The Midi Stream Parser
//
// ////////////////////////////////////////////////////////////////////////

static data byte rawMidiMode = RAW_MIDI;

static data byte msgBuf[3] = { 0,0,0 };
static data byte msgCnt = 0;
static data byte curCmd = 0;
#ifdef CONFIG_MidiSport2x2
static data byte msgBuf1[3] = { 0,0,0 };
static data byte msgCnt1 = 0;
static data byte curCmd1 = 0;
#endif

static code byte msgLenA[] = {                   3,3,3,3, 2,2,3   }; // 8x..Ex
static code byte msgLenB[] = { 3,2,3,2, 0,0,1,0, 1,0,1,1, 1,0,1,1 }; // F0..FF
static code byte cinTbl [] = { 5,2,3 };
static code byte cidlen [] = { 0,0,2,3, 3,1,2,3, 3,3,3,3, 2,2,3,1 };

static byte msgLen(byte cmd)
{
  return cmd < 0xf0 ? msgLenA[(cmd>>4)-8] : msgLenB[cmd&0x0f];
}

static byte cin(void)
{
  if (curCmd  < 0xf0) return (curCmd>>4);
  if (curCmd == 0xf0) return 4 + ((msgBuf[msgCnt-1] == 0xf7) ? msgCnt : 0);
  if (curCmd  < 0xf8) return cinTbl[msgCnt-1];
  return 15;
}

static void passPacket(byte dta, byte cid)
{
  inusbBuf[BUFPUT(inusbCtl)] = 0x0f | (cid<<4);
  inusbBuf[BUFPUT(inusbCtl)] = dta;
  inusbBuf[BUFPUT(inusbCtl)] = 0;
  inusbBuf[BUFPUT(inusbCtl)] = 0;
}

static void donePacket(byte cid)
{
  if (!msgLen(curCmd)) return; // undefined codes are indicated by length 0
  inusbBuf[BUFPUT(inusbCtl)] = cin() | (cid<<4);
  inusbBuf[BUFPUT(inusbCtl)] = msgBuf[0];
  inusbBuf[BUFPUT(inusbCtl)] = msgBuf[1];
  inusbBuf[BUFPUT(inusbCtl)] = msgBuf[2];
  msgCnt = msgBuf[0] = msgBuf[1] = msgBuf[2] = 0; // prepare for next message
  if (curCmd < 0xf0) msgBuf[msgCnt++] = curCmd;   // insert running status
  if (curCmd > 0xf0) curCmd = 0;                  // preserve running status
}

static void processByte(byte cc, byte cid)
{
  if (rawMidiMode) { passPacket(cc,cid); return; }
  if (cc > 0xf7) { if (msgLen(cc)) passPacket(cc,cid); return; }
  if (cc > 0x7f && curCmd == 0xf0) { msgBuf[msgCnt++] = 0xf7; donePacket(cid); }
  if (cc > 0x7f) { curCmd = cc; msgCnt = 0; }
  if (curCmd) msgBuf[msgCnt++] = cc;
  if (curCmd && msgCnt >= msgLen(curCmd)) donePacket(cid);
}

#ifdef CONFIG_MidiSport2x2
//FIXME: scanner pipeline fully cloned. This is pretty well suited to the 8051.
//       an earlier attempt to setup a controlling structure with a lot pointer
//       badly failed. We code in C, but for an 8 bit processor with very few ram.

static byte cin1(void)
{
  if (curCmd1  < 0xf0) return (curCmd1>>4);
  if (curCmd1 == 0xf0) return 4 + ((msgBuf1[msgCnt1-1] == 0xf7) ? msgCnt1 : 0);
  if (curCmd1  < 0xf8) return cinTbl[msgCnt1-1];
  return 15;
}

static void passPacket1(byte dta, byte cid)
{
  inusbBuf[BUFPUT(inusbCtl)] = 0x0f | (cid<<4);
  inusbBuf[BUFPUT(inusbCtl)] = dta;
  inusbBuf[BUFPUT(inusbCtl)] = 0;
  inusbBuf[BUFPUT(inusbCtl)] = 0;
}

static void donePacket1(byte cid)
{
  if (!msgLen(curCmd1)) return; // undefined codes are indicated by length 0
  inusbBuf[BUFPUT(inusbCtl)] = cin1() | (cid<<4);
  inusbBuf[BUFPUT(inusbCtl)] = msgBuf1[0];
  inusbBuf[BUFPUT(inusbCtl)] = msgBuf1[1];
  inusbBuf[BUFPUT(inusbCtl)] = msgBuf1[2];
  msgCnt1 = msgBuf1[0] = msgBuf1[1] = msgBuf1[2] = 0; // prepare for next message
  if (curCmd1 < 0xf0) msgBuf1[msgCnt1++] = curCmd1;   // insert running status
  if (curCmd1 > 0xf0) curCmd1 = 0;                    // preserve running status
}

static void processByte1(byte cc, byte cid)
{
  if (rawMidiMode) { passPacket1(cc,cid); return; }
  if (cc > 0xf7) { if (msgLen(cc)) passPacket1(cc,cid); return; }
  if (cc > 0x7f && curCmd1 == 0xf0) { msgBuf1[msgCnt1++] = 0xf7; donePacket1(cid); }
  if (cc > 0x7f) { curCmd1 = cc; msgCnt1 = 0; }
  if (curCmd1) msgBuf1[msgCnt1++] = cc;
  if (curCmd1 && msgCnt1 >= msgLen(curCmd1)) donePacket1(cid);
}
#endif

// //////////////////////////////////////////////////////
//
// TRANSITIONS
//
// //////////////////////////////////////////////////////

// MIDI -> USB Pipeline
// FIXME: extend pipeline to proper form
// Clear usb buffer as soon as the serial run full

// SBUF -> Ser -> ParserState -> USB

static void transSer2Usb(void)
{
  while ( LINCANWRITE(inusbCtl) && !BUFEMPTY(inserCtl) && !BUFFULL(inusbCtl))
  { byte dta = inserBuf[BUFGET(inserCtl)];
    processByte(dta,0x00);
  }
#ifdef CONFIG_MidiSport2x2
  while ( LINCANWRITE(inusbCtl) && !BUFEMPTY(inserCtl1) && !BUFFULL(inusbCtl))
  { byte dta = inserBuf1[BUFGET(inserCtl1)];
    processByte1(dta,0x01);
  }
#endif
  if ( LINCANWRITE(inusbCtl) && !BUFEMPTY(inusbCtl))
  {
    LINWRTDONE(inusbCtl);       // pass to consumer
    IN1BC = BUFCOUNT(inusbCtl); // arm ep
  }
}

// USB -> MIDI Pipeline
//
// USB -> Ser -> SBUF

static byte runningStatus  = 0;
#ifdef CONFIG_MidiSport2x2
static byte runningStatus1 = 0;
#endif

static void transUsb2Ser(void)
{
#ifdef CONFIG_MidiSport1x1
  if( LINCANREAD(outusbCtl) && !BUFEMPTY(outusbCtl) && !BUFFULL(outserCtl) )
  {
    byte pos = BUFGET(outusbCtl);
    byte len = cidlen[outusbBuf[pos&~0x03]&0x0f];
    byte dta = outusbBuf[pos];
    if ((pos&0x03) && (pos&0x03) <= len) outserBuf[BUFPUT(outserCtl)] = dta;
  }
#else
  if( LINCANREAD(outusbCtl) && !BUFEMPTY(outusbCtl) && !BUFFULL(outserCtl) && !BUFFULL(outserCtl1) )
  {
    byte pos = BUFGET(outusbCtl);
    byte len = cidlen[outusbBuf[pos&~0x03]&0x0f];
    byte cid = (outusbBuf[pos&~0x03]>>4)&0x0f;
    byte dta = outusbBuf[pos];
    if ((pos&0x03) && (pos&0x03) <= len)
    {
      if (cid==0) outserBuf[BUFPUT(outserCtl)] = dta;
      if (cid==1) outserBuf1[BUFPUT(outserCtl1)] = dta;
    }
  }
#endif
  if ( LINCANREAD(outusbCtl) && BUFEMPTY(outusbCtl) )
  {
    LINRDRDONE(outusbCtl); // pass to producer
    OUT1BC = 0;            // arm endpoint
  }
}

static void transSer2Uar(void)
{ // SERIAL -> UART
  if (!BUFEMPTY(outserCtl) && LINCANWRITE(outuarCtl))
  {
    byte dta = outserBuf[BUFGET(outserCtl)];
    // filter out running status bytes
    if (!rawMidiMode && runningStatus && dta == runningStatus) return;
    // maintain running status in any case
    if (dta > 0x7f) runningStatus = (dta < 0xf0) ? dta : 0;
    SBUF0 = dta; LINWRTDONE(outuarCtl);
  }
#ifdef CONFIG_MidiSport2x2
  if (!BUFEMPTY(outserCtl1) && LINCANWRITE(outuarCtl1))
  {
    byte dta = outserBuf1[BUFGET(outserCtl1)];
    // filter out running status bytes
    if (!rawMidiMode && runningStatus1 && dta == runningStatus1) return;
    // maintain running status in any case
    if (dta > 0x7f) runningStatus1 = (dta < 0xf0) ? dta : 0;
    SBUF1 = dta; LINWRTDONE(outuarCtl1);
  }
#endif
}

// //////////////////////////////////////////////////////
//
// USB and other utility stuff
//
// //////////////////////////////////////////////////////

static void SpinDelay(unsigned int count)
{
  while(count > 0) count -= 1;
}

static void ReEnumberate(void)
{
  USBCS &= ~0x04;
  USBCS |=  0x08;
  USBCS |= 0x02;
  SpinDelay(0xf401);
  USBCS &= ~0x08;
  USBCS |=  0x04;
}

static void doSuspend(void)
{
  USBBAV |= 0x08;
  do
  {
    USBCS |=  0x80;
    PCON  |=  0x01;
  }
  while (!bitRemoteWakeup && (USBCS & 0x80));
  if (USBCS & 0x80)
  {
    USBCS |=  0x01;     // |= SIGRSUME
    SpinDelay(0x1400);
    USBCS &= ~0x01;     // &= ~SIGRSUME
  }
  bitSUSPSeen = 0;
}

// ------------------------

static void setLeds(void)
{
  ledUSB = (USBFRAMEH>>2) == ( (((USBFRAMEH<<3)|(USBFRAMEL>>5))&0x1f)
                             > (USBFRAMEL&0x1f) );

#ifdef CONFIG_MidiSport1x1
  if ((USBFRAMEL&0x0f) == 0) ledIN = ledOUT = 0; // 1000/32   Hz = ca. 31 Hz

  OUTC = (ledIN?0:0x20)|(ledOUT?0:0x40)|(ledUSB?0:0x10)|0x03;
#else
  if ((USBFRAMEL&0x0f) == 0) ledIN = ledOUT = ledIN1 = ledOUT1 = 0; // 1000/32   Hz = ca. 31 Hz

  // 0x08: In2
  // 0x10: Out1
  // 0x20: In1
  // 0x40: USB
  // 0x04: Out2
  OUTC = (byte)((ledUSB?0:0x40)|(ledIN?0:0x20)|(ledOUT?0:0x10)|(ledIN1?0:0x08)|(ledOUT1?0:0x04)|0x03);
#endif
}

// //////////////////////////////////////////////////////
//
// MAIN
//
// //////////////////////////////////////////////////////

void main(void) // Never Terminates
{
  configuration = 0;

  initPipes ();
  initUSB   ();
  initPorts ();
  initSerial();
#ifdef HAS_TIMER_0
  initTimer ();
#endif

  EA = 1;

  // done init, reenumberate

  //FIXME quite some strange effects here for 1x1.
  // We can simply ReEnumberate, but will never be
  // able to read anything from the midi pipeline then.
  // 
  // This is aparently due to enabling serial interrupts before
  // reenumberation, as this blows up the pipes when uart read
  // events come in. No quick workaround in the code, though.
  // Workaround at the device is to unplung MIDI IN when downloading
  // the firmware.

  ReEnumberate();

  // Main Event Loop

  for (;;)
  {
    // Standard USB Material (EP0 Standard)
    if (bitSUDAVSeen) doSETUP();  // Handle SUDAV Events
    if (bitSUSPSeen) doSuspend(); // Handle SUSP Events

    // refresh Leds
    setLeds();

    // UART -> USB pipeline
    transSer2Usb();

    // USB -> UART pipeline
    transUsb2Ser(); transSer2Uar();
  }
}
