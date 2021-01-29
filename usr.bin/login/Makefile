TOPDIR = $(realpath ../..)

PROGRAM = login
SOURCES = login.c common.c
FORMAT-EXCLUDE = $(SOURCES)

include $(TOPDIR)/build/build.prog.mk

CFLAGS += -Wno-format-nonliteral
