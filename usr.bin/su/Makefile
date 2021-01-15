TOPDIR = $(realpath ../..)

PROGRAM = su
SOURCES = su.c suutil.c
FORMAT-EXCLUDE = $(SOURCES)
SETUID = 1

include $(TOPDIR)/build/build.prog.mk

CFLAGS += -Wno-format-nonliteral
LDLIBS = -lutil
