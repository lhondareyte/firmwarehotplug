/*
 *  Copyright (C) 2002 Lars Dölle <lars.doelle@online.de>
 *  Ported to ALSA by Pedro López-Cabanillas <plcl@bigfoot.com>
 *
 *  Loopback regression test for ALSA RawMIDI devices
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
#include <alsa/asoundlib.h>

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

static byte seq_aft[] = { 0xd0, 0x77, 0xd1, 0x66, 0xd2, 0x55, 0xd3, 0x44 };

/* Bn cc vv = control change
  (channel mode messages: local on, reset ctls, notes off) */

static byte seq_ctl[] = { 0xb0, 0x7a, 0x7f, 0xb1, 0x79, 0x00, 0xb2, 0x7b, 0x00 };

/* En ll mm = pitch bend */

static byte seq_bnd[] = { 0xe3, 0x21, 0x43, 0xe4, 0x00, 0x40, 0xe5, 0x41, 0x33 };

/* Cn pp = program change */

static byte seq_prg[] = { 0xc4, 0x6b, 0xc5, 0x41, 0xc0, 0x00, 0xc1, 0x0b };

/* An pp vv = polyphonic aftertouch */

static byte seq_paft[] = { 0xa1, 0x45, 0x50, 0xa2, 0x45, 0x00, 0xa3, 0x40, 0x01 };

struct test_case {
	char *name;
	char *comment;
	byte *bytes;
	int  length;
} test_case_t;

static struct test_case testseq[] = {
	{name: "seq_sysrt", comment: "system real time",      bytes: seq_sysrt,  length: sizeof(seq_sysrt)},
	{name: "seq_syx",   comment: "system exclusive",      bytes: seq_syx,    length: sizeof(seq_syx)},
	{name: "seq_sysc",  comment: "system common",         bytes: seq_sysc,   length: sizeof(seq_sysc)},
	{name: "seq_note",  comment: "note on/off",           bytes: seq_note,   length: sizeof(seq_note)},
	{name: "seq_aft",   comment: "channel aftertouch",    bytes: seq_aft,    length: sizeof(seq_aft)},
	{name: "seq_ctl",   comment: "control change",        bytes: seq_ctl,    length: sizeof(seq_ctl)},
	{name: "seq_bnd",   comment: "pitch bend",            bytes: seq_bnd,    length: sizeof(seq_bnd)},
	{name: "seq_prg",   comment: "program change",        bytes: seq_prg,    length: sizeof(seq_prg)},
	{name: "seq_paft",  comment: "polyphonic aftertouch", bytes: seq_paft,   length: sizeof(seq_paft)}
};

#define MAX_TEST sizeof(testseq) / sizeof(test_case_t) - 1

char *devin = "hw:0,0";
char *devout = NULL;
snd_rawmidi_t *handle_in = 0,*handle_out = 0;

int fd_in;
int fd_out;
int verbose = 0;
int test_number = 0;
int repeat_times = 1;

void usage()
{
	fprintf(stderr,
		"Loopback test for ALSA RawMIDI devices\n"
		"Usage: atestlb [OPTIONS] [-i hw:c,d,s [-o hw:c,d,s]]\n"
		"  -h, --help             this message\n"
		"  -i, --input=hw:C,D,S   test input C=card,D=device,S=subdevice\n"
		"  -o, --output=hw:C,D,S  test output C=card,D=device,S=subdevice\n"
		"  -r, --repeat=#         repeat test # times\n"
		"  -t, --test=#           select test number # (0..%d)\n"
		"  -v, --verbose          verbose mode\n", MAX_TEST);
}

void openall()
{
	int err;
	struct pollfd *midiifds = NULL;

	err = snd_rawmidi_open(&handle_in,NULL,devin,SND_RAWMIDI_NONBLOCK);
	if (err) {
		fprintf(stderr,"snd_rawmidi_open %s failed: %d [%s]\n", devin, err, snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	err = snd_rawmidi_open(NULL,&handle_out,devout,SND_RAWMIDI_NONBLOCK);
	if (err) {
		fprintf(stderr,"snd_rawmidi_open %s failed: %d [%s]\n", devout, err, snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	midiifds = (struct pollfd *)alloca(sizeof(struct pollfd));
	snd_rawmidi_poll_descriptors(handle_in, midiifds,1);
	fd_in = midiifds[0].fd;
	snd_rawmidi_poll_descriptors(handle_out, midiifds,1);
	fd_out = midiifds[0].fd;
}

void closeall()
{
	snd_rawmidi_drain(handle_in);
	snd_rawmidi_close(handle_in);

	snd_rawmidi_drain(handle_out);
	snd_rawmidi_close(handle_out);
}

int short_sequence_test(char *name, char *comment, byte * seq, int seqlen,
			int times)
{
	int len = seqlen * times;
	int putcnt = 0;
	int getcnt = 0;
	int maxfd = -1;
	int rc;
	struct timeval tv;
	fd_set rfds, wfds;

	if (verbose)
		printf(" TEST %s : %s : ", name, comment);

	/* Wait up to 100ms */
	tv.tv_sec = 0;
	tv.tv_usec = 100 * 1000;

	while (1) {

		maxfd = -1;

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

		rc = select(maxfd + 1, &rfds, &wfds, NULL, &tv);
		if (rc < 0) {
			perror(" Select failed at sst");
			exit(EXIT_FAILURE);
		} else if (rc > 0) {
			if (FD_ISSET(fd_in, &rfds)) {
				byte cc;

				if (getcnt >= putcnt) {
					fprintf(stderr,
						" ERROR1 %s getcnt(%d) >= putcnt(%d)\n", name, getcnt, putcnt);
					return 1;
				}
				snd_rawmidi_read(handle_in, &cc, 1);
				if (getcnt && getcnt % 100 == 0 && verbose) {
					printf(" %d", getcnt);
					fflush(stdout);
				}
				if (cc != seq[getcnt % seqlen]) {
					fprintf(stderr,
						" ERROR2 %s(%d) expecting 0x%02x, but got 0x%02x\n",
						name, getcnt,
						seq[getcnt % seqlen], cc);
					return 2;
				}
				getcnt += 1;
			}
			if (FD_ISSET(fd_out, &wfds)) {
				if (getcnt == putcnt && putcnt == len) {
					if (verbose)
						printf(" OK(?)\n");
					return 0;
				}
				snd_rawmidi_write(handle_out, seq + putcnt % seqlen, 1);
				putcnt += 1;
				if ((putcnt % seqlen) == 0) {
					snd_rawmidi_drain(handle_out);
				}
			}
		} else if (rc == 0) {
			if (putcnt < len) {
				fprintf(stderr,
					" ERROR3 %s : only %d of %d bytes written\n",
					name, putcnt, len);
				return 3;
			} else if (putcnt == getcnt) {
				if (verbose)
					printf(" OK\n");
				return 0;
			} else {
				fprintf(stderr,
					" ERROR4 %s : missing %d bytes at end of sequence.\n",
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
		{0, 0, 0, 0}
	};

	while (1) {
		c = getopt_long(argc, argv, "hi:o:r:t:v",
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
			if (test_number<0 || test_number>MAX_TEST) {
				fprintf(stderr, "Invalid test number: %d\n", test_number);
				return 1;
			}
			break;
		case 'v':
			verbose = 1;
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

	rc = short_sequence_test(
		testseq[test_number].name,
		testseq[test_number].comment,
		testseq[test_number].bytes,
		testseq[test_number].length,
		repeat_times
	);

	if (rc && verbose) {
		printf("FAIL (%d)\n", rc);
	}
	closeall();
	return rc;
}
