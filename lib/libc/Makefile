# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

CPPFLAGS = -I$(CURDIR) \
	   -I$(CURDIR)/include \
	   -I$(CURDIR)/citrus \
	   -I$(CURDIR)/gdtoa \
	   -I$(CURDIR)/gen \
	   -I$(CURDIR)/locale \
	   -I$(CURDIR)/quad \
	   -I$(CURDIR)/stdlib \
	   -D_LIBC -DNO_FENV_H


SUBLIBS = cdb citrus gdtoa gen locale misc regex quad \
	  stdio stdlib string sys termios time

include $(TOPDIR)/config.mk

SOURCES = $(foreach dir, $(SUBLIBS), \
            $(wildcard $(dir)/*.[cS]) $(wildcard $(dir)/$(ARCH)/*.[cS]))

include $(TOPDIR)/build/build.lib.mk

$(SOURCES): $(ARCH)/assym.h

CLEAN-FILES += $(ARCH)/assym.h
