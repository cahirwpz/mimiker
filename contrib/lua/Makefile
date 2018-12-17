TOPDIR = $(realpath ../..)

PKG = lua-5.3.5
URL = https://www.lua.org/ftp/$(PKG).tar.gz

all: download build install

download-here: $(PKG)

$(PKG).tar.gz:
	@echo "[CURL] $(PKG)"
	$(CURL) -# -O $(URL)

$(PKG): $(PKG).tar.gz
	@echo "[TAR] $^ -> $@"
	$(TAR) xzf $(PKG).tar.gz

build-here: download
	@echo "[MAKE] $(PKG)"
	$(MAKE) -C $(PKG) generic \
	  CC="$(CC)" AR="$(AR) rcu" RANLIB="$(RANLIB)" \
	  MYCFLAGS="$(CFLAGS)" MYLDFLAGS="$(LDFLAGS)"

install-here:
	@echo "[INSTALL] /bin/lua"
	$(INSTALL) -m 755 lua-5.3.5/src/lua $(SYSROOT)/bin/lua
	@echo "[STRIP] /bin/lua"
	$(STRIP) --strip-all $(SYSROOT)/bin/lua

clean-here:
	$(RM) -r $(PKG)

distclean-here:
	$(RM) $(PKG).tar.gz

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/common.mk
