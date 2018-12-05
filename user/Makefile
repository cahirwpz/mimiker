# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

SUBDIR = libmimiker newlib ls_rec mandelbrot sandbox test_kbd test_rtc utest

all: $(SUBDIR) install
install: $(SUBDIR-install)

# Dependencies between user programs are currently expressed here.
newlib: | libmimiker-install
ls_rec: | newlib-install
mandelbrot: | newlib-install
sandbox: | newlib-install
test_kbd: | newlib-install
test_rtc: | newlib-install
utest: | newlib-install

include $(TOPDIR)/build/subdir.mk
