# vim: tabstop=8 shiftwidth=8 noexpandtab:

include ../Makefile.common

SUBDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d -not -name sysroot)

.PHONY: clean sysroot all $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS): sysroot
	$(MAKE) -C $@

sysroot: sysroot/lib/libmimiker.a
sysroot/lib/libmimiker.a: start.o syscalls.o

clean:
	rm -f *.o sysroot/lib/*.a
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done
