TOPDIR = $(realpath ../..)

PROGRAM = stty
SOURCES = cchar.c gfmt.c key.c modes.c print.c stty.c
FORMAT-EXCLUDE = $(SOURCES)

include $(TOPDIR)/build/build.prog.mk

WFLAGS += -Wno-string-plus-int
