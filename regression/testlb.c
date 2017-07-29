/*
 *  Copyright (C) 2002 Lars Dölle <lars.doelle@online.de>
 *
 *  modified by Pedro López-Cabanillas <plcl@bigfoot.com>
 *  LoopBack regression test for MIDI devices
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>

typedef unsigned char byte;

/* We have to make the parser resistent wrt. Running Status & Sys */

/* System Real time messages:
   f8 = timing clock
   fa = start
   fb = continue
   fc = stop
   fe = active sensing
   ff = reset */

static byte seq_sysrt[] = { 0xf8, 0xfa, 0xfb, 0xfc, 0xff };

/* Non-commercial SYX message: id=0x7d */

static byte seq_syx[] =
    { 0xf0, 0x7d, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0xf7 };

/* System common
   f3 ss = song select
   f6 = tuning req. */

static byte seq_sysc[] = { 0xf3, 0x01, 0xf6 };

/* channel messages:
   9n pp vv = note on
   8n pp vv = note off */

static byte seq_note[] = { 0x90, 0x45, 0x7f, 0x80, 0x45, 0x40 };

/* Dn vv = channel aftertouch */

static byte seq_aft[] = { 0xd2, 0x77, 0xd3, 0x44, 0xd1, 0x00 };

/* Bn cc vv = control change
  (channel mode messages: local on, reset ctls, notes off) */

static byte seq_ctl[] =
    { 0xb0, 0x7a, 0x7f, 0xb1, 0x79, 0x00, 0xb2, 0x7b, 0x00 };

/* En ll mm = pitch bend */

static byte seq_bnd[] = { 0xe3, 0x21, 0x43, 0xe1, 0x00, 0x40 };

/* Cn pp = program change */

static byte seq_prg[] = { 0xc4, 0x6b, 0xc3, 0x41, 0xc2, 0x00 };

/* An pp vv = polyphonic aftertouch */

static byte seq_paft[] = { 0xa1, 0x45, 0x50, 0xa2, 0x45, 0x00 };


#define sst(X,C) short_sequence_test(#X,C,X,sizeof(X),1)
#define rst(X,C,T) short_sequence_test(#X,C,X,sizeof(X),T)
int short_sequence_test(char *name, char *comment, byte * seq, int seqlen,
			int times);

#define HERE printf("%s(%d): here\n",__FILE__,__LINE__)

char *devin = "/dev/midi";
char *devout = NULL;
int fd_in;
int fd_out;
int verbose = 0;
int dosync = 0;
int test_number = 0;
int repeat_times = 1;

void usage()
{
	fprintf(stderr,
		"Usage: testlb [OPTIONS] [-i /dev/midiIN [-o /dev/midiOUT]]\n"
		"Loopback test for MIDI devices\n"
		"  -h, --help         this message\n"
		"  -i, --input=NODE   test input device\n"
		"  -o, --output=NODE  test output device\n"
		"  -r, --repeat=#     repeat test # times\n"
		"  -t, --test=#       select test number #\n"
		"  -v, --verbose      verbose mode\n"
		"  -s, --sync         synchronize before test\n");
}

void openall()
{
	fd_in = open(devin, O_RDONLY);
	if (fd_in < 0) {
		perror(" Open input failed");
		exit(EXIT_FAILURE);
	}
	fd_out = open(devout, O_WRONLY);
	if (fd_out < 0) {
		perror(" Open output failed");
		exit(EXIT_FAILURE);
	}
}

void closeall()
{
	close(fd_in);
	close(fd_out);
}

/* BUG 1
   When we first open the device, we might get old data from
   some buffers. As MIDI is a real time infrastructure, this
   is illegal. The issue is located in the ezusbmidi firmware.
   We also use this as a workaround to flush input with some
   tests. */

int flush_input()
{
	int count, rc;
	byte cc;
	fd_set rfds;
	struct timeval tv;

	for (count = 0;; count++) {
		FD_ZERO(&rfds);
		FD_SET(fd_in, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;
		rc = select(fd_in + 1, &rfds, NULL, NULL, &tv);
		if (rc == 0)
			break;
		else if (rc < 0) {
			perror(" Select failed at flush_input()");
			return rc;
		} else if (rc > 0) {
			read(fd_in, &cc, 1);
			if (verbose)
				printf(" skipped buffered data: %02x\n", cc);
		}
		if (count > 100) {
			fprintf(stderr,
				" More than %d data. aborting.\n",
				count - 1);
			break;
		}
	}
	if (verbose)
		printf(" returning count: %d\n", count);
	return count;
}

/* BUG 2
   now, the usb-midi driver might drop first bytes send after reopen.
   we address this issue by sending/recognizing a sync sequence.
   this allows us also to exercise Real Time Messages.
   To this end, we send 0xf8, (0xf9), 0xfa, 0xfb, 0xfc, (0xfd), 0xfe, 0xff
   The data we receive should be one of them (otherwise we start over)
   until we can send/receive the complete sequence. */

int synchronize()
{
	int count;

	for (count = 0;; count++) {
		flush_input();
		if (sst(seq_sysrt, "system real time") == 0)
			break;
		if ((count > 5) || (test_number == 2)) {
			fprintf(stderr, " Synchronize failed\n");
			return 1;
		}
	}
	return 0;
}

/* BUG 3
   Poll (select) is aparently broken in two ways:
   it may signal data available to read (though there are non)
   and block then (correctly) the next read. It may also (and that
   happens with read too), simply block without delivering data.
   These problems are aparently triggered by open/close calls,
   even on other devices.

   error return codes:
   1:  POLL Problem, got more data than put
   2:  got data != put data
   3:  put less than len bytes
   4:  missing bytes at end
*/

int short_sequence_test(char *name, char *comment, byte * seq, int seqlen,
			int times)
{
	int len = seqlen * times;
	int putcnt = 0;
	int getcnt = 0;
	int maxfd = -1;
	int rc;

	if (verbose)
		printf(" TEST %s : %s : ", name, comment);

	for (;;) {
		fd_set rfds;
		fd_set wfds;
		struct timeval tv;

		// Watch stdin
		FD_ZERO(&rfds);
		FD_SET(fd_in, &rfds);
		maxfd = fd_in > maxfd ? fd_in : maxfd;

		// Watch fd_out while we have data
		FD_ZERO(&wfds);
		if (putcnt < len) {
			FD_SET(fd_out, &wfds);
			maxfd = fd_out > maxfd ? fd_out : maxfd;
		}

		/* Wait up to 100ms */
		tv.tv_sec = 0;
		tv.tv_usec = 100 * 1000;

		rc = select(maxfd + 1, &rfds, &wfds, NULL, &tv);
		/* Don't rely on the value of tv now! */
		if (rc < 0) {
			perror(" Select failed at sst");
			exit(EXIT_FAILURE);
		} else if (rc > 0) {
			if (FD_ISSET(fd_in, &rfds)) {
				byte cc;

				if (getcnt >= putcnt) {
					fprintf(stderr,
						" POLL problem?\n");
					return 1;
				}
				read(fd_in, &cc, 1);
				if (getcnt && getcnt % 100 == 0 && verbose) {
					printf(" %d", getcnt);
					fflush(stdout);
				}
				if (cc != seq[getcnt % seqlen]) {
					fprintf(stderr,
						" ERROR %s(%d) expecting 0x%02x, but got 0x%02x\n",
						name, getcnt,
						seq[getcnt % seqlen], cc);
					return 2;
				}
				getcnt += 1;
			}
			if (FD_ISSET(fd_out, &wfds)) {
				if (getcnt == putcnt && putcnt == len) {
					if (verbose)
						printf(" OK?\n");
					return 0;
				}
				write(fd_out, seq + putcnt % seqlen, 1);
				putcnt += 1;
			}
		} else if (rc == 0) {
			if (putcnt < len) {
				fprintf(stderr,
					" ERROR %s : only %d of %d bytes written\n",
					name, putcnt, len);
				return 3;
			} else if (putcnt == getcnt) {
				if (verbose)
					printf(" OK\n");
				return 0;
			} else {
				fprintf(stderr,
					" ERROR %s : missing %d bytes at end of sequence.\n",
					name, putcnt - getcnt);
				return 4;
			}
		}
	}
	return 0;
}

int parse_options(int argc, char *argv[])
{
	int c;
	int option_index = 0;
	static struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"input", 1, 0, 'i'},
		{"output", 1, 0, 'o'},
		{"repeat", 1, 0, 'r'},
		{"test", 1, 0, 't'},
		{"verbose", 0, 0, 'v'},
		{"sync", 0, 0, 's'},
		{0, 0, 0, 0}
	};

	while (1) {
		c = getopt_long(argc, argv, "hi:o:r:t:vs",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			devin = optarg;
			break;
		case 'o':
			devout = optarg;
			break;
		case 'r':
			repeat_times = atoi(optarg);
			break;
		case 't':
			test_number = atoi(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case 's':
			dosync = 1;
			break;
		case 0:
		case 'h':
		default:
			return 1;
		}
	}
	if (devout == NULL)
		devout = devin;
	return 0;
}

int main(int argc, char *argv[])
{
	int rc = 0;
	if (parse_options(argc, argv) != 0) {
		usage();
		return EXIT_FAILURE;
	}
	openall();
	switch (test_number) {
	case 1:
		rc = flush_input();
		break;
	case 2:
		rc = synchronize();
		break;
	case 3:
		if (dosync) synchronize();
		rc = rst(seq_syx, "system exclusive", repeat_times);
		break;
	case 4:
		if (dosync) synchronize();
		rc = rst(seq_sysc, "system common", repeat_times);
		break;
	case 5:
		if (dosync) synchronize();
		rc = rst(seq_note, "note on/off", repeat_times);
		break;
	case 6:
		if (dosync) synchronize();
		rc = rst(seq_aft, "aftertouch", repeat_times);
		break;
	case 7:
		if (dosync) synchronize();
		rc = rst(seq_ctl, "control change", repeat_times);
		break;
	case 8:
		if (dosync) synchronize();
		rc = rst(seq_bnd, "pitch bend", repeat_times);
		break;
	case 9:
		if (dosync) synchronize();
		rc = rst(seq_prg, "program change", repeat_times);
		break;
	case 10:
		if (dosync) synchronize();
		rc = rst(seq_paft, "polyphonic aftertouch", repeat_times);
		break;
	}
	closeall();
	return rc;
}
