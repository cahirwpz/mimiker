# vim: tabstop=8 shiftwidth=8 noexpandtab:

include ../Makefile.common

PROGDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d)
PROGDIRS-install = $(PROGDIRS:%=%-install)

.PHONY: clean all $(PROGDIRS)

all: $(PROGDIRS) install

install: $(PROGDIRS-install)

$(PROGDIRS):
	# @echo "Maybe compiling $@..." # Diagnostic message
	$(MAKE) -C $@

# .inst targets represent the installation of a program into sysroot.
%-install: %
	# @echo "Maybe installing $@..." # Diagnostic message
	$(MAKE) -C $< install

# Dependencies between user programs are currently expressed here.
newlib: | libmimiker-install
sandbox: | newlib-install
mandelbrot: | newlib-install
ls_rec: | newlib-install
utest: | newlib-install
test_kbd: | newlib-install
test_rtc: | newlib-install

clean:
	for dir in $(PROGDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done
