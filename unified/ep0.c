// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>

#include <ezusb_reg.h>
#include <bufsync.h>
#include <uart.h>
#include <desc.h>

// ///////////////////////////////////////////////////////////////////////
//
// EP 0 Control Protocol
//
// ///////////////////////////////////////////////////////////////////////

#define IsSelfPowerDevice 0
static byte bitRemoteWakeup; // = 0; // deactivated at reset

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

static byte configuration; // 1 if in configured state

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
  //FIXME: issue stall if configuration > 1 (we should accept 0,1)
  configuration = SETUPDAT.wValueL; // alternate
  EP0CS = HSNAK; // Acknowledge Control transfer
}

static void ctrlSetInterface(void)
{
  //FIXME: issue stall on invalid arguments
//       0 == SETUPDAT.wIndexL; // interface
//interface = SETUPDAT.wValueL; // al
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

//FIXME: SDCC problem. cannot move this routine to [desc.c]
static xdata byte* findDescriptor(byte key, byte index)
{ unsigned int p; byte count = 0;
  for (p = 0; Desc[p]; p+=Desc[p])
    if (Desc[p+1] == key && count++ == index)
      return &Desc[p];
  return 0;
}

static void ctrlGetDescriptor(void)
{ xdata byte* d = findDescriptor(SETUPDAT.wValueH,SETUPDAT.wValueL);
  if (!d) { EP0CS = STALL; return; }
  SUDPTRH = (byte)((((unsigned int)d))>>8)&0xff;
  SUDPTRL = (byte)(  (unsigned int)d     )&0xff;
  EP0CS = HSNAK; // Acknowledge Control transfer
}

// Diagnostics for I2C material

static void ctrlDiagnostics(void)
{ byte i; extern xdata byte eeprom[];
  for (i = 0; i < 8; i++) IN0BUF[i] = eeprom[i];
  IN0BC = 0x08;
  EP0CS = HSNAK; // Acknowledge Control transfer
}

void doSETUP(void)
{
  switch  (SETUPDAT.bRequest)
  {
    case 0x00: ctrlGetStatus();         break;
    case 0x01: ctrlClearFeature();      break;
//  case 0x02: /*  reserved */          break; // unsupported
    case 0x03: ctrlSetFeature();        break;
//  case 0x04: /* reserved */           break; // unsupported
    case 0x05: /*  SetAddress */        break; //FIXME: (test) handled by EZUSB core
//  case 0x66: ctrlGetDescriptor();     break;
    case 0x06: ctrlGetDescriptor();     break;
//  case 0x07: /*  SetDescriptor */     break; // unsupported
    case 0x08: ctrlGetConfiguration();  break;
    case 0x09: ctrlSetConfiguration();  break;
    case 0x0a: ctrlGetInterface();      break;
    case 0x0b: ctrlSetInterface();      break;
    case 0x0c: /* Sync      */          break; //FIXME: (test) handled by EZUSB core

    case 0x66: ctrlDiagnostics();       break; //private firmware diagnostics

    default  : EP0CS = STALL;           break;
  }
  USBIEN |= 0x02;
}

// //////////////////////////////////////////////////////
//
// other USB stuff
//
// //////////////////////////////////////////////////////

static void SpinDelay(unsigned int count)
{
  while(count > 0) count -= 1;
}

void ReEnumberate(void)
{
  USBCS &= ~0x04;
  USBCS |=  0x08;
  USBCS |= 0x02;
  SpinDelay(0xf401);
  USBCS &= ~0x08;
  USBCS |=  0x04;
}

void doSuspend(void)
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
}

static void isrWakeup(void) //interrupt FIXME: reactivate
{
  EICON &= 0xef;
}

void initEP0(void)
{
  configuration = 0;
  bitRemoteWakeup = 0;
}
