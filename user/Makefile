# vim: tabstop=8 shiftwidth=8 noexpandtab:

include ../Makefile.common

PROGDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d)

.PHONY: clean all $(PROGDIRS)

all: $(PROGDIRS)

$(PROGDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(PROGDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done
