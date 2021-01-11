TOPDIR = $(realpath ../..)

ATTO_URL = https://github.com/hughbarney/atto.git
INCURSES_URL = https://github.com/infinnovation-dev/incurses.git

PROGRAM = atto
BINDIR = usr/bin
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

FORMAT-EXCLUDE = $(SOURCES)

# XXX Build fails if download is called for the first time.

all: download build install

atto:
	@echo "[GIT] clone $(ATTO_URL) -> $@"
	$(GIT) clone $(ATTO_URL) $@

incurses:
	@echo "[GIT] clone $(INCURSES_URL) -> $@"
	$(GIT) clone $(INCURSES_URL) $@

download-here: atto incurses

include $(TOPDIR)/build/build.prog.mk

CPPFLAGS += -Iincurses -UDEBUG
WFLAGS += -Wno-strict-prototypes -Wno-implicit-fallthrough -Wno-sign-compare \
          -Wno-char-subscripts
