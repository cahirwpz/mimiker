TOPDIR = $(realpath ../..)

PROGRAM = atto
BINDIR = usr/bin
BUILDDIR = build/
SOURCES = incurses/incurses.c \
	  atto/command.c \
	  atto/display.c \
	  atto/gap.c \
	  atto/key.c \
	  atto/search.c \
	  atto/buffer.c \
	  atto/replace.c \
	  atto/window.c \
	  atto/complete.c \
	  atto/hilite.c \
	  atto/main.c

include $(TOPDIR)/build/build.prog.mk

download-here: atto/.git incurses/.git

build-before: download

CPPFLAGS += -Iincurses -UDEBUG
WFLAGS += -Wno-strict-prototypes -Wno-implicit-fallthrough -Wno-sign-compare \
          -Wno-char-subscripts
