/*
 *  Copyright (C) 2002 Pedro Lopez-Cabanillas <plcl@bigfoot.com>
 *
 *  Based on rawmidi, from ALSA test package
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
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define ON	1
#define OFF	0
#define IGNORE -1

typedef unsigned char byte;

int stop = OFF;
int thru = OFF;
int verbose = OFF;
int iotest = ON;
char *node_in = 0;
char *node_out = 0;
int fd_in = IGNORE;
int fd_out = IGNORE;

void usage()
{
	fprintf(stderr, "Usage: midiraw [OPTIONS]\n"
		"Input and output raw MIDI device test\n"
		"  -h, --help           this message\n"
		"  -i, --input [node]   test input device\n"
		"  -o, --output [node]  test output device\n"
		"  -n, --noio           test open only\n"
		"  -t, --thru           test midi thru\n"
		"  -v, --verbose        verbose mode\n");
}

int parse_options(int argc, char *argv[])
{
	int c;
	int option_index = 0;
	static struct option long_options[] = {
		{"help", 0, 0, 'h'},
		{"input", 1, 0, 'i'},
		{"output", 1, 0, 'o'},
		{"thru", 0, 0, 't'},
		{"noio", 0, 0, 'n'},
		{"verbose", 0, 0, 'v'},
		{0, 0, 0, 0}
	};

	while (1) {
		c = getopt_long(argc, argv, "hi:o:tnv",
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'i':
			node_in = optarg;
			break;
		case 'o':
			node_out = optarg;
			break;
		case 't':
			thru = ON;
			break;
		case 'n':
			iotest = OFF;
			break;
		case 'v':
			verbose = ON;
			break;
		case 0:
		case 'h':
		default:
			return 1;
		}
	}
	return 0;
}

void do_open()
{
	if (verbose) {
		fprintf(stderr, "Using: \n");
		fprintf(stderr, "Input: ");
		if (node_in)
			fprintf(stderr, "%s\n", node_in);
		else
			fprintf(stderr, "NONE\n");
		fprintf(stderr, "Output: ");
		if (node_out)
			fprintf(stderr, "%s\n", node_out);
		else
			fprintf(stderr, "NONE\n");
	}

	if (node_in && (!node_out || strcmp(node_out, node_in))) {
		fd_in = open(node_in, O_RDONLY);
		if (fd_in < 0) {
			if (verbose)
				perror("Open input failed");
			exit(EXIT_FAILURE);
		}
	}

	if (node_out && (!node_in || strcmp(node_out, node_in))) {
		fd_out = open(node_out, O_WRONLY);
		if (fd_out < 0) {
			if (verbose)
				perror("Open output failed");
			exit(EXIT_FAILURE);
		}
	}

	if (node_in && node_out && strcmp(node_out, node_in) == 0) {
		fd_in = fd_out = open(node_out, O_RDWR);
		if (fd_out < 0) {
			if (verbose)
				perror("Open input and output failed");
			exit(EXIT_FAILURE);
		}
	}
}

void do_close()
{
	if (verbose) {
		fprintf(stderr, "Closing\n");
	}
	if (fd_in != IGNORE) {
		close(fd_in);
	}
	if (fd_out != IGNORE) {
		close(fd_out);
	}
}

void test_in()
{
	byte ch;
	if (fd_in != IGNORE) {
		fprintf(stderr, "Reading midi in\n");
		fprintf(stderr, "Press ctrl-c to stop\n");
		while (!stop) {
			read(fd_in, &ch, sizeof(ch));
			if (verbose) {
				printf(" %02x", ch);
			}
		}
	}
}

void test_out()
{
	byte noteon[] = { 0x90, 0x45, 0x70 };
	byte noteoff[] = { 0x80, 0x45, 0x70 };
	if (fd_out != IGNORE) {
		if (verbose)
			fprintf(stderr, "Writing note on / note off\n");
		write(fd_out, noteon, sizeof(noteon));
		sleep(1);
		write(fd_out, noteoff, sizeof(noteon));
	}
}

void test_thru()
{
	byte ch;
	if ((fd_in != IGNORE) && (fd_out != IGNORE)) {
		fprintf(stderr, "Testing midi thru in\n");
		fprintf(stderr, "Press ctrl-c to stop\n");
		while (!stop) {
			if (fd_in != IGNORE) {
				read(fd_in, &ch, sizeof(ch));
			}
			if (verbose) {
				if (ch != 0xf7 && ch > 0x7f)
					printf("\n");
				printf(" %02x", ch); fflush(stdout);
			}
			if (fd_out != IGNORE) {
				write(fd_out, &ch, sizeof(ch));
			}
		}
	} else {
		fprintf(stderr,
			"Testing midi thru needs both input and output\n");
		exit(EXIT_FAILURE);
	}
}


void sighandler(int dum)
{
	stop = ON;
}

int main(int argc, char *argv[])
{
	if (parse_options(argc, argv) != 0) {
		usage();
		return EXIT_FAILURE;
	}
	do_open();
	if (iotest) {
		signal(SIGINT, sighandler);
		if (thru) {
			test_thru();
		} else {
			test_in();
			test_out();
		}
	}
	do_close();
	return EXIT_SUCCESS;
}
