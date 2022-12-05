TOPDIR = $(realpath ../..)

PKG = lua

all: download build install

download-here: $(PKG)/.git

$(PKG)/.git:
	@echo "[GIT] submodule $(PKG)"
	$(GIT) submodule update --init $(PKG)

patch: download .pc/done
.pc/done:
	@echo "[QUILT] apply patches"
	quilt push -a
	touch $@

build-here: patch
	@echo "[MAKE] $(PKG)"
	$(MAKE) -C $(PKG) all \
	  CC="$(CC)" AR="$(AR) rcu" LD="$(LD)" RANLIB="$(RANLIB)" \
	  MYCFLAGS="$(CPPFLAGS) $(CFLAGS) -I$(realpath .)/$(PKG)" \
	  MYLDFLAGS="$(LDFLAGS)" MYLIBS="$(LDLIBS)"

install-here:
	@echo "[INSTALL] /bin/lua"
	$(INSTALL) -m 755 $(PKG)/lua $(SYSROOT)/bin/lua
	@echo "[OBJCOPY] $(SYSROOT)/bin/lua -> \
		$(SYSROOT)/bin/lua.dbg"
	$(OBJCOPY) --only-keep-debug $(SYSROOT)/bin/lua \
		$(SYSROOT)/bin/lua.dbg
	@echo "[STRIP] /bin/lua"
	$(STRIP) --strip-all $(SYSROOT)/bin/lua
	@echo "[INSTALL] /lib/lua/init.lua"
	$(INSTALL) -D -m 644 init.lua $(SYSROOT)/lib/lua/init.lua

clean-here:
	@echo "[MAKE] clean $(PKG)"
	-$(MAKE) -C $(PKG) clean
	@echo "[QUILT] remove patches"
	test -f .pc/done && quilt pop -a
	rm -f .pc/done

PHONY-TARGETS += patch unpatch

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/common.mk
