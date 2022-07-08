TOPDIR = $(realpath ../..)

PROGRAM = su
SOURCES = su.c suutil.c
FORMAT-EXCLUDE = $(SOURCES)
BINMODE = 4755
CFLAGS += -Wno-format-nonliteral
LDLIBS = -lutil

include $(TOPDIR)/build/build.prog.mk
