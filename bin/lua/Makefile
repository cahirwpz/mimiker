TOPDIR = $(realpath ../..)

PKG = lua-5.3.5
URL = https://www.lua.org/ftp/$(PKG).tar.gz
PATCHES = loslib.patch lunix.patch luaconf.patch init.patch

all: download build install

download-here: $(PKG)

$(PKG).tar.gz:
	@echo "[CURL] $(PKG)"
	$(CURL) -# -O $(URL)

$(PKG): $(PKG).tar.gz
	@echo "[TAR] $^ -> $@"
	$(TAR) xzf $(PKG).tar.gz
	touch $(PKG)

patch: download patch-stamp
patch-stamp:
	cd $(PKG) && \
	  for p in $(PATCHES); do \
	    $(PATCH) -p1 < ../$${p}; \
	  done
	ln -s -t $(PKG)/src ../../lunix.c
	touch $@

unpatch:
	if [ -f patch-stamp ]; then \
	  cd $(PKG) && \
	    for p in $(PATCHES); do \
	      $(PATCH) -R -p1 < ../$$p; \
	    done; \
	fi
	$(RM) $(PKG)/src/lunix.c
	$(RM) patch-stamp

build-here: patch
	@echo "[MAKE] $(PKG)"
	$(MAKE) -C $(PKG) generic \
	  CC="$(CC)" AR="$(AR) rcu" LD="$(LD)" RANLIB="$(RANLIB)" \
	  MYCFLAGS="$(CFLAGS)" MYLDFLAGS="$(LDFLAGS)" MYLIBS="$(LDLIBS)"

install-here:
	@echo "[INSTALL] /bin/lua"
	$(INSTALL) -m 755 lua-5.3.5/src/lua $(SYSROOT)/bin/lua
	@echo "[STRIP] /bin/lua"
	$(STRIP) --strip-all $(SYSROOT)/bin/lua
	@echo "[INSTALL] /lib/lua/init.lua"
	$(INSTALL) -D -m 644 init.lua $(SYSROOT)/lib/lua/init.lua

clean-here: unpatch
	$(RM) -r $(PKG)

distclean-here:
	$(RM) $(PKG).tar.gz

PHONY-TARGETS += patch unpatch

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/common.mk
