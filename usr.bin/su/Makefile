TOPDIR = $(realpath ../..)

PROGRAM = su
SOURCES = su.c suutil.c
FORMAT-EXCLUDE = $(SOURCES)

include $(TOPDIR)/build/build.prog.mk

CFLAGS += -Wno-format-nonliteral
LDLIBS = -lutil
