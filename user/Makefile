# vim: tabstop=8 shiftwidth=8 noexpandtab:

include ../Makefile.common

SUBDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d)

.PHONY: clean common all $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS): | common
	$(MAKE) -C $@

# This target may be invoked by make run in a user program subdirectory, so
# plase do not have it pull any dependencies outside this dir.
common: start.o syscalls.o

clean:
	rm -f start.o syscalls.o
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done
