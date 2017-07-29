VERSION = $(shell date +%Y_%m_%d)
RELEASE_NAME =	ezusbmidi-$(VERSION)

all: ezusbmidi1x1.ihx ezusbmidi2x2.ihx 

doc: ezusbmidi.html ezusbmidi.ps

ezusbmidi1x1.ihx: ezusbmidi.c ezusb_reg.h bufsync.h
	rm -rf build; mkdir build; cp $^ build
	cd build; sdcc -DCONFIG_MidiSport1x1 -I. $<; packihx ezusbmidi.ihx > ../$@
	cd ..

ezusbmidi2x2.ihx: ezusbmidi.c ezusb_reg.h bufsync.h
	rm -rf build; mkdir build; cp $^ build
	cd build; sdcc -DCONFIG_MidiSport2x2  -I. $<; packihx ezusbmidi.ihx > ../$@
	cd ..

ezusbmidi.ps: ezusbmidi.sgml descriptor.png
	sgml2latex --output=ps $<

ezusbmidi.html: ezusbmidi.sgml descriptor.png
	sgml2html $<

rpm: release
	rpm -ta $(RELEASE_NAME).tar.bz2

deb: release
	debuild binary

deb-release:
	fakeroot dpkg-buildpackage

clean:
	rm -rf build
	rm -f [1-9] *.pdf *.html *.ps *.ihx *.tar.gz *.tar.bz2 ezusbmidi.spec
	rm -rf debian/ezusbmidi debian/files build-stamp configure-stamp

install:
	mkdir -p $(DESTDIR)/etc/hotplug/usb
	mkdir -p $(DESTDIR)/usr/share/usb/ezusbmidi
	install -m755 Hotplug/ezusbmidi $(DESTDIR)/etc/hotplug/usb
	install -m644 Hotplug/ezusbmidi.usermap $(DESTDIR)/etc/hotplug/usb
	install -m644 ezusbmidi1x1.ihx $(DESTDIR)/usr/share/usb/ezusbmidi
	install -m644 ezusbmidi2x2.ihx $(DESTDIR)/usr/share/usb/ezusbmidi

DISTFILES = $(shell find . \( -not -name '.' \) -print | grep -v CVS | grep -v "\.tar\.bz2" | grep -v "\.tar\.gz" | grep -v "\/\." | grep -v releases | grep -v BitKeeper | grep -v SCCS ) ezusbmidi.spec
DISTDIR=$(RELEASE_NAME)

ezusbmidi.spec: ezusbmidi.spec.in
	sed 's/TODAY/$(VERSION)/g' ezusbmidi.spec.in > ezusbmidi.spec

release: clean $(DISTFILES)
	@echo $(DISTFILES)
	@-rm -rf $(DISTDIR)
	@mkdir $(DISTDIR)
	@-chmod 777 $(DISTDIR)
	@for file in $(DISTFILES); do			\
		if test -d $$file; then			\
		  	mkdir $(DISTDIR)/$$file;	\
		else					\
			cp -p $$file $(DISTDIR)/$$file;	\
		fi;					\
	done
	@tar -c $(DISTDIR) | bzip2 -9 > $(RELEASE_NAME).tar.bz2
	@rm -rf $(DISTDIR)
	@echo "Built $(RELEASE_NAME).tar.bz2"

freebsd-install:
	@pkg info fxload > /dev/null 2>&1 || (echo "Error: fxload not present"; exit 1)
	@mkdir -p /usr/local/etc/devd
	@mkdir -p /usr/local/share/fxload/m-audio
	@install -m 644 -o root -g wheel ezusbmidi1x1.ihx /usr/local/share/fxload/m-audio/
	@install -m 644 -o root -g wheel ezusbmidi2x2.ihx /usr/local/share/fxload/m-audio/
	@install -m 644 -o root -g wheel freebsd.conf /usr/local/etc/devd/m-audio.conf
	@service devd restart
