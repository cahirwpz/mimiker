TOPDIR = $(realpath ../..)

PKG = incurses
URL = https://github.com/infinnovation-dev/incurses
SRCDIR = $(PKG)
LIBFILE = libncursesw.a

all: download build install

$(PKG):
	@echo "[GIT] clone $(URL) -> $@"
	$(GIT) clone $(URL) $(PKG)

download-here: $(PKG)

$(PKG)/incurses.c: $(PKG)

build-here: $(PKG)/incurses.o

$(LIBFILE): $(PKG)/incurses.o

install-here: $(LIBFILE)

clean-here:
	$(RM) $(PKG)/incurses.o

distclean-here:
	$(RM) -rf $(PKG) $(LIBFILE)

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/common.mk

CPPFLAGS += -UDEBUG
