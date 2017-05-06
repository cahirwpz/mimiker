# vim: tabstop=8 shiftwidth=8 noexpandtab:

include ../Makefile.common

PROGDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d)
PROGDIRS-inst = $(PROGDIRS:%=%.inst)

.PHONY: clean all $(PROGDIRS)

all: $(PROGDIRS) install

install: $(PROGDIRS-inst)

$(PROGDIRS):
	@echo "Maybe compiling $@..."
	$(MAKE) -C $@

# .inst targets represent the installation of a program into sysroot.
%.inst: %
	@echo "Maybe installing $@..."
	$(MAKE) -C $< install


# Dependencies between user programs are currently expressed here.
newlib: | libmimiker.inst
prog: | newlib.inst
fd_test: | newlib.inst
misbehave: | newlib.inst
mandelbrot: | newlib.inst
test_fork: | newlib.inst

clean:
	for dir in $(PROGDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done
