// [bufsync.h] Synchronizing buffer
// This file is part of the ezusbmidi package.
// Copyright (c) 2001 by Lars Doelle <lars.doelle@on-line.de>
//
// This algorithm implements a cyclic BUFfer able to synchronize
// two asynchronous producer and consumer processes without the
// need of a semaphore or any other other atomic synchronizer.
//
// If the consumer only does a BUFGET when the BUFGET precondition
// is valid, but the producer cannot invalidate this condition once
// true, and both the consumer and producer manipulate distinct
// variables, the algorithm is save for the consumer. The same
// proof holds vis versa for the producer.
//
// The algorithm uses modul arithmetic.
//
// Note that since (N mod (B*A)) mod A = (N mod A) mod (B*A),
// we can use 'mod 2*size' for the arithmetic of the reader
// and writer variables to gain BUFCOUNT in range of 0..BUFMAX
// inclusively, while 'mod size' provides the correct indices.
//
// The subalgorithm here uses sizes which are powers of 2.
// In this case, modul arithmetic for incrementing the reader
// and writer variables is done by the machine arithmetic implicitly,
// while the modulo for the indexes reduces to a simple binary and
// operation. Likely, the BUFCOUNT reduces to a subtraction.

struct CYCSYN
{
  byte rdr;
  byte wrt;
};

#define BUFEXP 6
#define BUFMAX (1<<(BUFEXP)) // = 64 = sizeof(INxBUF)
#define BUFMSK (BUFMAX-1)

#define BUFCOUNT(BUF) (BUF.wrt-BUF.rdr)
#define BUFFULL(BUF)  (BUFCOUNT(BUF) == BUFMAX)
#define BUFEMPTY(BUF) (BUFCOUNT(BUF) == 0)
#define BUFFREE(BUF)  (BUFMAX - BUFCOUNT(BUF))

//FIXME: sdcc post-increment is broken when using records.
//FIXME: work around: (++BUF.rdr-1) means BUF.rdr++

#define BUFGET(BUF)  ((++BUF.rdr-1) & BUFMSK)
#define BUFPUT(BUF)  ((++BUF.wrt-1) & BUFMSK)

#define CYCINIT(BUF) BUF.rdr = BUF.wrt = 0

// The EZUSB endpoint buffer are non-cyclic, so we cannot synchronize
// in the above way. Instead, we have to use a semaphore, which indicates
// the the buffer is in use by either the producer or be consumer.
// The semaphore can be set by a process only while the process is holding it.

struct LINSYN
{
  char rdr;
  char wrt;
  byte sem;
};

#define LININIT(BUF) BUF.rdr = BUF.wrt = BUF.sem = 0
#define LINWRTDONE(BUF) BUF.rdr = 0; BUF.sem = 1
#define LINRDRDONE(BUF) LININIT(BUF)
#define LINCANREAD(BUF) BUF.sem == 1
#define LINCANWRITE(BUF) BUF.sem == 0
