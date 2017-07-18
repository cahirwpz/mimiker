# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

PROGDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d)
PROGDIRS-install = $(PROGDIRS:%=%-install)

all: $(PROGDIRS) install

install: $(PROGDIRS-install)

$(PROGDIRS):
	$(MAKE) -C $@

# install targets represent the installation of a program into sysroot.
%-install: %
	$(MAKE) -C $< install

# Dependencies between user programs are currently expressed here.
newlib: | libmimiker-install
sandbox: | newlib-install
mandelbrot: | newlib-install
ls_rec: | newlib-install
test_execve: | newlib-install
utest: | newlib-install
test_kbd: | newlib-install
test_rtc: | newlib-install

clean:
	for dir in $(PROGDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done

.PHONY: $(PROGDIRS)

include $(TOPDIR)/build/common.mk
