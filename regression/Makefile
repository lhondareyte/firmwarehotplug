#
# Regression tests for usb-midi Makefile
#
CFLAGS=-Wall -g

all: testlb midimsg midispy midiraw

install:
	echo "NOT installing regression tests anywhere. (for now)"

testlb.o: testlb.c

midimsg.o: midimsg.c

midispy.o: midispy.c ansicolor.h

midiraw.o: midiraw.c

test:	programs
	$(SHELL) ./alltests.sh

loopback: programs
	$(SHELL) ./loopback.sh

instrument: programs
	$(SHELL) ./instrument.sh

clean:
	rm -f testlb midimsg midispy midiraw
	rm -f *.o
	rm -f testlog.*
