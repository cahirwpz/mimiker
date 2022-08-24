TOPDIR = $(realpath ../..)

PROGRAM = su
SOURCES = su.c suutil.c
FORMAT-EXCLUDE = $(SOURCES)
BINMODE = 4755
LDLIBS = -lutil

include $(TOPDIR)/build/build.prog.mk

WFLAGS += -Wno-format-nonliteral
