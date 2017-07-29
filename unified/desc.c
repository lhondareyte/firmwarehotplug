// [desc.c] Generate Multi-Jack Descriptor
// Copyright (c) 2001,2002 by Lars Doelle <lars.doelle@on-line.de>
//
// "gcc -o Desc -DSTAND_ALONE desc.c" to dry-test descriptors

#ifdef STAND_ALONE
typedef unsigned char byte;
#else
#include <ezusb_reg.h>
#endif
#define AT(X) __at X
#define code __code
#define xdata __xdata

#define EMBEDDED 0x01
#define EXTERNAL 0x02
#define XXXX     0x00 // variable (vendorid, productid, config len, iface len)

#define COPYRIGHT "Copyright (GPLv2) 2001 by Lars Doelle <lars.doelle@on-line.de>"

static code byte Base[] =
{
//FIXME check bmAttributes & MaxPower
  0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x40, XXXX, // Device ...
  XXXX, XXXX, XXXX, 0x00, 0x00, 0x01, 0x02, 0x03, 0x01, //  ...
  0x09, 0x02, XXXX, XXXX, 0x02, 0x01, 0x00, 0x00, 0x32, // Config
  0x09, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, // Interface 0
  0x09, 0x24, 0x01, 0x00, 0x01, 0x09, 0x00, 0x01, 0x01, // CS Interface (audio)
  0x09, 0x04, 0x01, 0x00, 0x02, 0x01, 0x03, 0x00, 0x00, // Interface 1
  0x07, 0x24, 0x01, 0x00, 0x01, XXXX, XXXX,             // CS Interface (midi)
  0x09, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, // Endpoint OUT
  0x09, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00, 0x00, 0x00, // Endpoint IN
  0x04, 0x03, 0x09, 0x04,                               // String Lang:EN
  0x00
};

// Note that SUDPTR cannot operate on external memory.
// Thus, when using SUDPTR, as we do in ep0/getDescriptor,
// one has place the descriptors either in internal ram,
// code space or in the EP buffer space.
//
// This consumes valueable endpoint buffer space and leaves
// only EP1 available for transfers.

xdata byte AT(0x7b40) Desc[0x300]; // EP IN/OUT BUFFER 7-2
static unsigned int DescLen;

static void addByte(byte x) { Desc[DescLen++] = x; }

static xdata byte* addDesc(byte index)
{ byte i; unsigned int pos0 = DescLen; code byte* p = Base;
  while (*p && index--) p+=*p;
  for (i = 0; i < *p; i++) addByte(p[i]);
  return &Desc[pos0];
}

static void addInJack(byte bJackType, byte bJackId)
{
  addByte(0x06); addByte(0x24); addByte(0x02);
  addByte(bJackType); addByte(bJackId); addByte(0x00);
}

static void addOutJack(byte bJackType, byte bJackId, byte baSourceId)
{
  addByte(0x09); addByte(0x24); addByte(0x03); addByte(bJackType); addByte(bJackId);
  addByte(0x01); addByte(baSourceId); addByte(0x01); addByte(0x00);
}

static void addCSEP(byte start, byte count)
{ byte i;
  addByte(count+4); addByte(0x25); addByte(0x01); addByte(count);
  for (i = 0; i < count; i++) addByte(start+4*i);
}

static void addText(code char* str)
{
  unsigned int pos = DescLen;
  addByte(0); addByte(3);
  while (*str) { addByte(*str++); addByte(0); }
  Desc[pos] = DescLen-pos;
}

/*

             ... Mux over all i ports (cables)
               /--
              |
              |     4*i + 3          4*i + 2
   EP_1_IN  <-#--- Out Jack Emb <-- IN  Jack Ext

   EP_1_OUT --#--> In  Jack Emb --> OUT Jack Ext
              |     4*i + 1          4*i + 4
              |
               \-> 
             ... Demux over all i ports (cables)

*/

void makeDesc(byte ports, code char* name, xdata byte* ids)
{ xdata byte* devd; xdata byte* cnfd; xdata byte* ifcd; xdata byte* eod;
  unsigned int len1; unsigned int len2; byte i;
  DescLen = 0;
  devd = addDesc(0);                     // Device Desc
  cnfd = addDesc(1);                     // Config Desc
  addDesc(2);                            // IF 0 (Audio)
  addDesc(3);                            // IF CS
  addDesc(4);                            // IF 1 (Midi)
  ifcd = addDesc(5);                     // IF CS
  for (i = 0; i < ports; i++)
  {
    addInJack (EMBEDDED, 4*i+1);          //  IN  Jack Emb
    addInJack (EXTERNAL, 4*i+2);          //  IN  Jack Ext
    addOutJack(EMBEDDED, 4*i+3, 4*i+2);   //  OUT Jack Emb <- IN Ext
    addOutJack(EXTERNAL, 4*i+4, 4*i+1);   //  OUT Jack Ext <- IN Emb
  }
  addDesc(6);                             // EP 1 OUT
  addCSEP(1,ports);                       //   EP CS :: IN Emb
  addDesc(7);                             // EP 1 IN
  addCSEP(3,ports);                       //   EP CS :: Out Emb
  eod = addDesc(8);                       // Language Code
  addText("Midiman");                     // iManufacturer
  addText(name);                          // iProduct
  addText(COPYRIGHT);                     // iSerial
  addByte(0);                             // End
  len1 = eod-cnfd; cnfd[2] = (len1>>0)&0xff; cnfd[3] = (len1>>8)&0xff;
  len2 = eod-ifcd; ifcd[5] = (len2>>0)&0xff; ifcd[6] = (len2>>8)&0xff;
  for (i = 0; i < 4; i++) devd[8+i] = ids[i]; // patch vendor/product
  devd[10] += 1; // increment product id
}

#ifdef STAND_ALONE
#include <stdio.h>
byte ids[] = { 0x47, 0x11, 0x08, 0x15 };
int main(int argc, char* argv[])
{ int p;
  makeDesc(argc>1?atoi(argv[1]):1,argc>2?argv[2]:"ezusbmidi",ids);
  for (p = 0; Desc[p]; p+=Desc[p])
  { int len = Desc[p]; int i;
    printf("/* %3d */",p);
    for (i = 0; i < len; i++)
    {
      if (i && i%10==0) printf("\n/* ... */");
      printf(" 0x%02x,",Desc[p+i]);
    }
    printf("\n");
  }
  printf("/* %3d */ 0x00\n",p);
  return 0;
}
#endif
