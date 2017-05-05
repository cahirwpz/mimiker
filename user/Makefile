# vim: tabstop=8 shiftwidth=8 noexpandtab:

include ../Makefile.common

PROGDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d)
PROGDIRS-inst = $(PROGDIRS:%=%.inst)

.PHONY: clean all $(PROGDIRS)

all: $(PROGDIRS) install

install: $(PROGDIRS-inst)

$(PROGDIRS):
	$(MAKE) -C $@

%.inst: %
	$(MAKE) -C $< install

clean:
	for dir in $(PROGDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done
