TOPDIR = $(realpath ../..)

BUILTINS =
INTERNALS = env.lua ls.lua 

build-here: mish

mish: builtin.lua $(BUILTINS) internal.lua $(INTERNALS) shell.lua
	@echo "[CAT] $^ -> $@"
	echo '#!/bin/lua\n' > $@
	cat $^ >> $@

include $(TOPDIR)/build/common.mk

install-here:
	echo "[INSTALL] /bin/mish"
	$(INSTALL) -m 755 mish $(SYSROOT)/bin/mish

clean-here:
	$(RM) mish
