SUBDIRS=original regression unified

all:
	@for DIR in $(SUBDIRS); do	\
		make -C $$DIR all;	\
	done

clean:
	@for DIR in $(SUBDIRS); do	\
		make -C $$DIR clean;	\
	done

install:
	@for DIR in $(SUBDIRS); do	\
		make -C $$DIR install;	\
	done
