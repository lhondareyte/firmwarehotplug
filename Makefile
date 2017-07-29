SUBDIRS=original regression unified
MAKE = gmake

all:
	@for DIR in $(SUBDIRS); do	\
		$(MAKE) -C $$DIR all;	\
	done

clean:
	@for DIR in $(SUBDIRS); do	\
		$(MAKE) -C $$DIR clean;	\
	done

install:
	@for DIR in $(SUBDIRS); do	\
		$(MAKE) -C $$DIR install;	\
	done
