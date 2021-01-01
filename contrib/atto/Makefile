TOPDIR = $(realpath ../..)

PKG = atto
URL = https://github.com/hughbarney/atto.git
INCURSES_URL = https://github.com/infinnovation-dev/incurses.git
INCURSES_DIR = incurses
INCURSES_PATH = $(realpath $(INCURSES_DIR))
INCURSES_LIBFILE = $(INCURSES_DIR)/libncursesw.a
SRCDIR = $(PKG)
PKG-CFLAGS = -I$(INCURSES_PATH)
PKG-LDFLAGS = -L$(INCURSES_PATH)

all: download build install

$(PKG):
	@echo "[GIT] clone $(URL) -> $@"
	$(GIT) clone $(URL) $(PKG)

$(INCURSES_DIR):
	@echo "[GIT] clone $(INCURSES_URL) -> $@"
	$(GIT) clone $(INCURSES_URL) $@

download-here: $(PKG) $(INCURSES_DIR)

$(INCURSES_DIR)/incurses.c: $(INCURSES_DIR)

$(INCURSES_LIBFILE): $(INCURSES_DIR)/incurses.o

build-here: $(INCURSES_LIBFILE)
	@echo "[MAKE] build $(SRCDIR)"
	$(MAKE) -C $(SRCDIR) CC="$(CC)" LD="$(LD)" \
	  CFLAGS="$(CPPFLAGS) $(CFLAGS) $(PKG-CFLAGS)" \
	  LDFLAGS="$(PKG-LDFLAGS) $(LDFLAGS)"

install-here:
	@echo "[INSTALL] $(SRCDIR)/atto -> /bin/atto"
	$(INSTALL) -T $(SRCDIR)/atto $(SYSROOT)/bin/atto
	@echo "[STRIP] /bin/atto"
	$(STRIP) -s $(SYSROOT)/bin/atto

clean-here:
	@echo "[MAKE] clean $(SRCDIR)"
	$(MAKE) -C $(SRCDIR) clean RM="$(RM)"
	$(RM) $(INCURSES_LIBFILE) $(INCURSES_DIR)/incurses.o

distclean-here:
	$(RM) -r $(PKG) $(INCURSES_DIR)

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/common.mk

CPPFLAGS += -UDEBUG
