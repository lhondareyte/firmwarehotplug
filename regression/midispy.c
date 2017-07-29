/*
 *  Copyright (C) 2002 Lars Dölle <lars.doelle@online.de>
 *
 *  Decode and print out MIDI events
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "ansicolor.h"

#include <sys/time.h>
#include <time.h>

// Time Profiles ///////////////////////////////////////////////////////

typedef unsigned char byte;

int fd;
int stop;

char *msgnames[] = {
  ANSI_GREEN "Note off    ",
  ANSI_HIGH_GREEN "Note on     ",
  ANSI_HIGH_BLUE "Poly aft .  ",
  ANSI_HIGH_RED "Control chg ",
  ANSI_HIGH_YELLOW "Program chg ",
  ANSI_HIGH_CYAN "Aftertouch  ",
  ANSI_HIGH_MAGENTA "Pich bend   ",
  ANSI_HIGH_WHITE "System      "
};

void displaystatus(byte status)
{
  int i = ((status & 0xf0) >> 4) - 8;
  if (i < 7)
    printf("%02i %s", (status & 0x0f) + 1, msgnames[i]);
  else
    printf("-- %s", msgnames[i]);
}

long timeMS()
{
  struct timeval tv; gettimeofday(&tv,0);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int mainloop()
{
  byte data[100];
  int rt = 0;
  int cnt = 0;
  long basetime = timeMS();
  long delta0   = 0;
  stop = 0;
  while (!stop)
  {
    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 100 * 1000;
    rt = select(fd + 1, &rfds, NULL, NULL, &tv);
    if (rt < 0)
    {
      perror("\nselect");
      break;
    }
    if (rt)
    { long delta; int i;
      int rc = read(fd, data, sizeof(data));
      if (rc <= 0) break;
      delta = timeMS() - basetime;
      for (i = 0; i < rc; i++)
      { byte dta = data[i];
        if (1 || dta != 0xfe)
        {
          if (dta != 0xf7 && dta > 0x7f)
          {
            printf(ANSI_NORMAL "\n%7.3f : %5ld : %04x ", delta/1000.0, delta-delta0, cnt);
            displaystatus(dta);
          }
          printf(" %02x", dta);
        }
        cnt++;
      }
      fflush(stdout);
      delta0 = delta;
    }
  }
  return rt;
}

void sighandler(int dum)
{
  stop = 1;
}

int main(int argc, char *argv[])
{
  char *device = argc > 1 ? argv[1] : "/dev/midi";
    signal(SIGINT, sighandler);

  fd = open(device, O_RDONLY | O_NONBLOCK, 0);
  if (fd < 0) {
    perror(__FILE__ " error at open");
    return EXIT_FAILURE;
  }
  printf("opened %s, waiting for data.\n", device);
  printf("press ^C to exit\n");

  mainloop();

        printf("\n%s",ANSI_NORMAL); fflush(stdout);
  if (close(fd) < 0) {
    perror(__FILE__ " error at close");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
