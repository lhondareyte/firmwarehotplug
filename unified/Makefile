CFLAGS=--model-large -I. # --stack-after-data
all: ezusbmidi.ihx

ezusbmidi.ihx: ezusbmidi.c ezusb_reg.h bufsync.h uart.c uart.h ep0.c desc.c desc.h ep0.h pipeline.c pipeline.h leds.h Makefile ezusbmidi.h
	rm -rf build;\
	 mkdir build;\
	 cp $^ build;\
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
