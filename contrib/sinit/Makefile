TOPDIR = $(realpath ../..)

PROGRAM = init
BINDIR = sbin
BUILDDIR = build/
SOURCES = sinit/sinit.c

include $(TOPDIR)/build/build.prog.mk

build-before: quilt-patch

download-here: sinit/.git

clean-here: quilt-unpatch

$(OBJECTS): $(BUILDDIR)sinit/config.h

$(BUILDDIR)%.h: %.def.h
	cp $< $@

CPPFLAGS += -I$(BUILDDIR)/sinit
