// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>
#include <ezusbmidi.h>
#include <ezusb_reg.h>
#include <bufsync.h>
#include <uart.h>
#include <leds.h>

// Here come the variable for the Petri net. /////////////////////////

//FIXME: make this a proper Endpoint struct
//FIXME: make more functions static
// USB EP (in/out)

static xdata struct LINSYN inusbCtl;        // EP 1 IN control
static xdata at 0x7e80 byte inusbBuf[0x40];

// EP 1 OUT -> UART (0) IN pipeline

static xdata struct LINSYN  outusbCtl;       // EP 1 OUT control
static xdata at 0x7e40 byte outusbBuf[0x40];

static byte rawMidiMode; // = 0; //FIXME: per Pipe?

struct Pipe
{
  struct LINSYN  outUarCtl;       // corresponds to Ext Plug (UART semaphore)
  struct CYCSYN  inSerCtl;        // UART->serial (controls Emb Plug)
  struct CYCSYN  outSerCtl;       // serial->UART (controls Emb Plug)
  byte inSerBuf[0x40];            // corresponds Emb Plug (Midi Stream)
  byte outSerBuf[0x40];           // corresponds Emb Plug (Midi Stream)
  //
  // Variables for the MIDI -> USB-MIDI transducer
  //
  byte MsgBuf[3]; // = { 0,0,0 };
  byte MsgCnt; // = 0;
  byte CurCmd; // = 0;
  //
  // Variables for the USB-MIDI -> MIDI transducer
  //
  byte RunningStatus; //  = 0;
  //
  //
  //
  byte cid; // Cabel Id
};

xdata struct Pipe pipe[MAX_PORTS];
xdata struct Uart uart[MAX_PORTS];

// ////////////////////////////////////////////////////////////////////////
//
// The Midi Stream Parser
//
// ////////////////////////////////////////////////////////////////////////

static code byte msgLenA[] = {                   3,3,3,3, 2,2,3   }; // 8x..Ex
static code byte msgLenB[] = { 3,2,3,2, 0,0,1,0, 1,0,1,1, 1,0,1,1 }; // F0..FF
static code byte cinTbl [] = { 5,2,3 };
static code byte cidlen [] = { 0,0,2,3, 3,1,2,3, 3,3,3,3, 2,2,3,1 };

static byte msgLen(byte cmd)
{
  return cmd < 0xf0 ? msgLenA[(cmd>>4)-8] : msgLenB[cmd&0x0f];
}

static byte cin(xdata struct Pipe* p)
{
  if (p->CurCmd  < 0xf0) return (p->CurCmd>>4);
  if (p->CurCmd == 0xf0) return 4 + ((p->MsgBuf[p->MsgCnt-1] == 0xf7) ? p->MsgCnt : 0);
  if (p->CurCmd  < 0xf8) return cinTbl[p->MsgCnt-1];
  return 15;
}

static void passPacket(xdata struct Pipe* p, byte dta)
{
  inusbBuf[BUFPUT(inusbCtl)] = 0x0f | (p->cid<<4);
  inusbBuf[BUFPUT(inusbCtl)] = dta;
  inusbBuf[BUFPUT(inusbCtl)] = 0;
  inusbBuf[BUFPUT(inusbCtl)] = 0;
}

static void donePacket(xdata struct Pipe* p)
{
  if (!msgLen(p->CurCmd)) return; // undefined codes are indicated by length 0
  inusbBuf[BUFPUT(inusbCtl)] = cin(p) | (p->cid<<4);
  inusbBuf[BUFPUT(inusbCtl)] = p->MsgBuf[0];
  inusbBuf[BUFPUT(inusbCtl)] = p->MsgBuf[1];
  inusbBuf[BUFPUT(inusbCtl)] = p->MsgBuf[2];
  p->MsgCnt = p->MsgBuf[0] = p->MsgBuf[1] = p->MsgBuf[2] = 0; // prepare for next message
  if (p->CurCmd < 0xf0) p->MsgBuf[p->MsgCnt++] = p->CurCmd;   // insert running status
  if (p->CurCmd > 0xf0) p->CurCmd = 0;                        // preserve running status
}

static void processByte(xdata struct Pipe* p, byte cc)
{
  if (rawMidiMode) { passPacket(p,cc); return; }
  if (cc > 0xf7) { if (msgLen(cc)) passPacket(p,cc); return; }
  if (cc > 0x7f && p->CurCmd == 0xf0) { p->MsgBuf[p->MsgCnt++] = 0xf7; donePacket(p); }
  if (cc > 0x7f) { p->CurCmd = cc; p->MsgCnt = 0; }
  if (p->CurCmd) p->MsgBuf[p->MsgCnt++] = cc;
  if (p->CurCmd && p->MsgCnt >= msgLen(p->CurCmd)) donePacket(p);
}

// //////////////////////////////////////////////////////
//
// BOTTOM HALF ISRs
//
// //////////////////////////////////////////////////////

void isrUartBottom(byte port)
{
  if (uart[port].ti)
  {
    uart[port].ti = 0;
    ledOUT(port) ^= 1;
    if (LINCANREAD(pipe[port].outUarCtl)) LINRDRDONE(pipe[port].outUarCtl);
  }
  if (uart[port].ri)
  { byte dta = getPortData(port);
    uart[port].ri = 0;
    if (dta!=0xfe) ledIN(port) ^= 1; // ignore active-sensing
    if (!BUFFULL(pipe[port].inSerCtl)) pipe[port].inSerBuf[BUFPUT(pipe[port].inSerCtl)] = dta;
  }
}

void doEP1_ri()
{ 
  outusbCtl.wrt = OUT1BC; LINWRTDONE(outusbCtl);
}

void doEP1_ti()
{
  LINRDRDONE(inusbCtl);
}

// //////////////////////////////////////////////////////
//
// TRANSITIONS
//
// //////////////////////////////////////////////////////


// MIDI -> USB Pipeline
// FIXME: extend pipeline to proper form
// Clear usb buffer as soon as the serial run full

// SBUF -> Ser -> ParserState -> USB

static void transSer2Usb(byte ports)
{ byte i;
  for (i = 0; i < ports; i++)
  {
    while ( LINCANWRITE(inusbCtl) && !BUFEMPTY(pipe[i].inSerCtl) && !BUFFULL(inusbCtl))
    { byte dta = pipe[i].inSerBuf[BUFGET(pipe[i].inSerCtl)];
      processByte(&pipe[i],dta);
    }
  }
  if ( LINCANWRITE(inusbCtl) && !BUFEMPTY(inusbCtl))
  {
    LINWRTDONE(inusbCtl);       // pass to consumer
    IN1BC = BUFCOUNT(inusbCtl); // arm ep
  }
}

// USB -> MIDI Pipeline
//
// USB -> Ser -> SBUF

static void transUsb2Ser(byte ports)
{
  while( LINCANREAD(outusbCtl) && !BUFEMPTY(outusbCtl))
  {
    byte pos = BUFPEEK(outusbCtl);
    byte cid = (outusbBuf[pos&~0x03]>>4)&0x0f;
    byte len = cidlen[outusbBuf[pos&~0x03]&0x0f];
    byte dta = outusbBuf[pos];
    if (cid < ports)
    {
      if ( BUFFULL(pipe[cid].outSerCtl) ) break;
      if ((pos&0x03) && (pos&0x03) <= len)
          pipe[cid].outSerBuf[BUFPUT(pipe[cid].outSerCtl)] = dta;
    }
    BUFRINC(outusbCtl);
  }
  if ( LINCANREAD(outusbCtl) && BUFEMPTY(outusbCtl) )
  {
    LINRDRDONE(outusbCtl); // pass to producer
    OUT1BC = 0;            // arm endpoint
  }
}

static void transSer2Uar(byte ports)
{ // SERIAL -> UART
  byte i;
  for (i = 0; i < ports; i++)
  if (!BUFEMPTY(pipe[i].outSerCtl) && LINCANWRITE(pipe[i].outUarCtl))
  {
    byte dta = pipe[i].outSerBuf[BUFGET(pipe[i].outSerCtl)];
    // filter out running status bytes
    if (rawMidiMode && pipe[i].RunningStatus && dta == pipe[i].RunningStatus) continue;
    // maintain running status in any case
    if (dta > 0x7f) pipe[i].RunningStatus = (dta < 0xf0) ? dta : 0;
    putPortData(i,dta); LINWRTDONE(pipe[i].outUarCtl);
  }
}

static void initPort(byte i)
{
  pipe[i].RunningStatus = 0;
  pipe[i].MsgCnt        = 0;
  pipe[i].MsgBuf[0]     = 0;
  pipe[i].MsgBuf[1]     = 0;
  pipe[i].MsgBuf[2]     = 0;
  pipe[i].CurCmd        = 0;
  pipe[i].cid           = i; //FIXME: for simplicity
  ledIN(i)              = 0;
  ledOUT(i)             = 0;
  CYCINIT(pipe[i].inSerCtl);
  CYCINIT(pipe[i].outSerCtl);
  LININIT(pipe[i].outUarCtl);
}

void initPipes(byte ports)
{ byte i;
  //TODO: really serie over all EPs
  LININIT(inusbCtl);
  LININIT(outusbCtl);

  for (i = 0; i < ports; i++) initPort(i);

  rawMidiMode   = 0;
}

void runPipes(byte ports)
{
    // UART -> USB pipeline
    transSer2Usb(ports);

    // USB -> UART pipeline
    transUsb2Ser(ports);
    transSer2Uar(ports);
}
