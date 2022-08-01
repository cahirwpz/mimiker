# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SUBLIBS = builtins cdb citrus gdtoa gen locale misc regex \
	  stdio stdlib string sys termios time

include $(TOPDIR)/config.mk

SOURCES = $(foreach dir, $(SUBLIBS), \
            $(wildcard $(dir)/*.[cS]) $(wildcard $(dir)/$(ARCH)/*.[cS]))

include $(TOPDIR)/build/build.lib.mk

CPPFLAGS += -I$(CURDIR) \
	    -I$(CURDIR)/include \
	    -I$(CURDIR)/builtins \
	    -I$(CURDIR)/citrus \
	    -I$(CURDIR)/gdtoa \
	    -I$(CURDIR)/gen \
	    -I$(CURDIR)/locale \
	    -I$(CURDIR)/stdlib \
	    -D_LIBC -DNO_FENV_H

$(SOURCES): $(ARCH)/assym.h

CLEAN-FILES += $(ARCH)/assym.h
