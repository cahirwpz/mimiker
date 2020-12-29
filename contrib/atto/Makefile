TOPDIR = $(realpath ../..)

PKG = atto
URL = https://github.com/hughbarney/atto.git
SRCDIR = $(PKG)
CURSES_DIR = $(realpath ../incurses)
PKG-CFLAGS = -I$(CURSES_DIR)/incurses
PKG-LDFLAGS = -L$(CURSES_DIR)

all: download build install

$(PKG):
	@echo "[GIT] clone $(URL) -> $@"
	$(GIT) clone $(URL) $(PKG)

download-here: $(PKG)

build-here:
	$(MAKE) -C $(CURSES_DIR)
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
	$(MAKE) -C $(SRCDIR) clean

distclean-here:
	$(RM) -r $(PKG)

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/common.mk
