/*
 *  Copyright (C) 2002 Pedro Lopez-Cabanillas <plcl@bigfoot.com>
 *
 *  This program sends MIDI channel mode messages to external
 *  MIDI instruments like keyboards, synthesizers, etc.
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
#include <getopt.h>
#include <string.h>
#include <fcntl.h>

#define ON	1
#define OFF	2
#define MONO	1
#define POLY	2
#define GM	1
#define GS	2
#define XG	3

typedef unsigned char byte;

static byte msg_localon[] = { 0xb0, 0x7a, 0x7f };
static byte msg_localoff[] = { 0xb0, 0x7a, 0 };
static byte msg_modemono[] = { 0xb0, 0x7e, 0x01 };
static byte msg_modepoly[] = { 0xb0, 0x7f, 0 };
static byte msg_omnion[] = { 0xb0, 0x7d, 0 };
static byte msg_omnioff[] = { 0xb0, 0x7c, 0 };
static byte msg_reset[] = { 0xb0, 0x79, 0 };
static byte msg_shutup[] = { 0xb0, 0x7b, 0 };
static byte msg_gmreset[] = { 0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7 };
static byte msg_gsreset[] =
    { 0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0, 0x7f, 0, 0x41, 0xf7 };
static byte msg_xgreset[] =
    { 0xf0, 0x43, 0x10, 0x4c, 0, 0, 0x7e, 0, 0xf7 };

char *devicename = "/dev/midi";
char fromchan = 0;
char tochan = 15;
int doLocal = 0;
int doMode = 0;
int doOmni = 0;
int doReset = 0;
int doShutup = 0;
int doSyxreset = 0;
int midiout;

void usage()
{
	fprintf(stderr, "Usage: midimsg [OPTION]... [MIDI DEVICE]\n"
		"Send MIDI channel mode messages to MIDI DEVICE (default=/dev/midi)\n"
		"  -h, --help			this message\n"
		"  -c, --channel=0..15		use only one midi channel\n"
		"  -l, --local=on|off		local control message\n"
		"  -m, --mode=mono|poly		mode message\n"
		"  -o, --omni=on|off		omni message\n"
		"  -p, --panic			same as -rs\n"
		"  -r, --reset			reset all controllers\n"
		"  -s, --shutup			all notes off\n"
		"  -x, --syxreset=gm|gs|xg	system exclusive reset\n");
}

void send_msg(byte * seq, int len)
{
	if (write(midiout, seq, len) < 0) {
		perror(__FILE__ "error at send_msg()");
		exit(EXIT_FAILURE);
	}
}

void send_all_chan(byte * seq, int len)
{
	byte chan;
	for (chan = fromchan; chan <= tochan; chan++) {
		seq[0] &= 0xf0;
		seq[0] |= chan;
		send_msg(seq, len);
	}
}

void send_messages()
{
	if (doLocal == ON)
		send_all_chan(msg_localon, sizeof(msg_localon));

	if (doLocal == OFF)
		send_all_chan(msg_localoff, sizeof(msg_localoff));

	if (doMode == MONO)
		send_all_chan(msg_modemono, sizeof(msg_modemono));

	if (doMode == POLY)
		send_all_chan(msg_modepoly, sizeof(msg_modepoly));

	if (doOmni == ON)
		send_all_chan(msg_omnion, sizeof(msg_omnion));

	if (doOmni == OFF)
		send_all_chan(msg_omnioff, sizeof(msg_omnioff));

	if (doReset == ON)
		send_all_chan(msg_reset, sizeof(msg_reset));

	if (doShutup == ON)
		send_all_chan(msg_shutup, sizeof(msg_shutup));

	if (doSyxreset == GS)
		send_msg(msg_gsreset, sizeof(msg_gsreset));

	if (doSyxreset == GM)
		send_msg(msg_gmreset, sizeof(msg_gmreset));

	if (doSyxreset == XG)
		send_msg(msg_xgreset, sizeof(msg_xgreset));
}

int parse_options(int argc, char *argv[])
{
	int c;
	int option_index = 0;
	static struct option long_options[] = {
		{"channel", 1, 0, 'c'},
		{"help", 0, 0, 'h'},
		{"local", 1, 0, 'l'},
		{"mode", 1, 0, 'm'},
		{"omni", 1, 0, 'o'},
		{"panic", 0, 0, 'p'},
		{"reset", 0, 0, 'r'},
		{"shutup", 0, 0, 's'},
		{"syxreset", 1, 0, 'x'},
		{0, 0, 0, 0}
	};

	while (1) {
		c = getopt_long(argc, argv, "c:hl:m:o:prsx:",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'c':
			if (optarg && (strcmp(optarg, "all") != 0)) {
				fromchan = tochan = (char) atoi(optarg);
			}
			break;

		case 'l':
			if (strcmp(optarg, "on") == 0)
				doLocal = ON;
			else if (strcmp(optarg, "off") == 0)
				doLocal = OFF;
			else
				return 1;
			break;

		case 'm':
			if (strcmp(optarg, "mono") == 0)
				doMode = MONO;
			else if (strcmp(optarg, "poly") == 0)
				doMode = POLY;
			else
				return 1;
			break;

		case 'o':
			if (strcmp(optarg, "on") == 0)
				doOmni = ON;
			else if (strcmp(optarg, "off") == 0)
				doOmni = OFF;
			else
				return 1;
			break;

		case 'p':
			doReset = ON;
			doShutup = ON;
			break;

		case 'r':
			doReset = ON;
			break;

		case 's':
			doShutup = ON;
			break;

		case 'x':
			if (strcmp(optarg, "gm") == 0)
				doSyxreset = GM;
			else if (strcmp(optarg, "gs") == 0)
				doSyxreset = GS;
			else if (strcmp(optarg, "xg") == 0)
				doSyxreset = XG;
			else
				return 1;
			break;

		case 0:
		case 'h':
		default:
			return 1;
		}
	}
	if (optind < argc)
		devicename = argv[optind];
	return 0;
}

int main(int argc, char *argv[])
{
	if (parse_options(argc, argv) != 0) {
		usage();
		return EXIT_FAILURE;
	}
	midiout = open(devicename, O_WRONLY);
	if (midiout < 0) {
		perror(__FILE__ "error at open");
		return EXIT_FAILURE;
	}
	send_messages();
	if (close(midiout) < 0) {
		perror(__FILE__ "error at close");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
