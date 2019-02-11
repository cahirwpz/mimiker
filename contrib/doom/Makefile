TOPDIR = $(realpath ../..)

PKG = doom
URL = https://github.com/jwhitham/DOOM.git
SRCDIR = $(PKG)/headless_doom
DATADIR = /usr/share/doom
PKG-CFLAGS = -Wno-extra -Wno-strict-prototypes -Wno-unknown-pragmas \
	     -Wno-format-overflow
PATCHES = 01-vga-output.patch

all: download build install

download-here: $(PKG)

$(PKG):
	@echo "[GIT] clone $(URL) -> $@"
	$(GIT) clone $(URL) $(PKG)

patch: download patch-stamp
patch-stamp:
	@for p in $(PATCHES); do \
	  echo "[PATCH] apply $$p"; \
	  $(PATCH) -d $(SRCDIR) -p2 < $$p; \
	done
	touch $@

unpatch:
	@if [ -f patch-stamp ]; then \
	  for p in $(PATCHES); do \
	    echo "[PATCH] revert $$p"; \
	    $(PATCH) -d $(SRCDIR) -R -p2 < $$p; \
	  done; \
	fi
	$(RM) patch-stamp

build-here: patch-stamp
	@echo "[MAKE] build $(SRCDIR)"
	$(MAKE) -C $(SRCDIR) benchmark.exe doom.wad.ok DDQ-EP1.LMP \
	  CC="$(CC)" CFLAGS="$(CFLAGS) $(PKG-CFLAGS)" LDFLAGS="$(LDFLAGS)"

install-here:
	@echo "[INSTALL] $(SRCDIR)/benchmark.exe -> /bin/doom"
	$(INSTALL) -T $(SRCDIR)/benchmark.exe $(SYSROOT)/bin/doom
	@echo "[STRIP] /bin/doom"
	$(STRIP) -s $(SYSROOT)/bin/doom
	@echo "[INSTALL] $(SRCDIR)/doom.wad -> $(DATADIR)/doom.wad"
	$(INSTALL) -m 644 $(SRCDIR)/doom.wad -t $(SYSROOT)/$(DATADIR)
	@echo "[INSTALL] $(SRCDIR)/*.LMP -> $(DATADIR)/*.LMP"
	$(INSTALL) -m 644 $(SRCDIR)/*.LMP -t $(SYSROOT)/$(DATADIR)

clean-here:
	@echo "[MAKE] clean $(SRCDIR)"
	$(MAKE) -C $(SRCDIR) clean

distclean-here: unpatch
	$(RM) -r $(PKG)

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/common.mk

PHONY-TARGETS += patch unpatch
