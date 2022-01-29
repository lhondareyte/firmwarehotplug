CFLAGS  = --model-large -I. # --stack-after-data
SOURCES = ezusbmidi.c uart.c ep0.c desc.c pipeline.c ezusbmidi.h
HEADERS = ezusb_reg.h bufsync.h uart.h desc.h ep0.h pipeline.h leds.h ezusbmidi.h

all: ezusbmidi.ihx

ezusbmidi.ihx: Makefile $(SOURCES) $(HEADERS)
	rm -rf build;\
	 mkdir build;\
	 cp $(SOURCES) $(HEADERS) build;\
	 cd build;\
	 sdcc -c $(CFLAGS) desc.c;\
	 sdcc -c $(CFLAGS) pipeline.c;\
	 sdcc -c $(CFLAGS) ep0.c;\
	 sdcc -c $(CFLAGS) uart.c;\
	 sdcc -c $(CFLAGS) ezusbmidi.c;\
	 sdcc $(CFLAGS) --xram-loc 0x2000 ezusbmidi.rel uart.rel ep0.rel pipeline.rel desc.rel;\
	 packihx ezusbmidi.ihx > ../$@

clean:
	rm -rf build [1-9] ezusbmidi.ihx

install:
	echo "unified firmware not installed (for now)."

freebsd-install:
	@pkg info fxload > /dev/null 2>&1 || (echo "Error: fxload not present"; exit 1)
	@mkdir -p /usr/local/etc/devd
	@mkdir -p /usr/local/share/fxload/m-audio
	@install -m 644 -o root -g wheel ezusbmidi.ihx /usr/local/share/fxload/m-audio/
	@install -m 644 -o root -g wheel freebsd.conf /usr/local/etc/devd/m-audio.conf
	@service devd restart

