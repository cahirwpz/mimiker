TOPDIR = $(realpath ../..)

LUA_SRC = lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c \
	lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c \
	ltm.c lundump.c lvm.c lzio.c ltests.c lauxlib.c lbaselib.c ldblib.c \
	liolib.c lmathlib.c loslib.c ltablib.c lstrlib.c lutf8lib.c loadlib.c \
	lcorolib.c linit.c lua.c

SOURCES = $(addprefix lua/,$(LUA_SRC)) lunix.c
PROGRAM = lua
BINDIR = bin
BUILDDIR = build/
FORMAT-EXCLUDE = $(SOURCES)

CPPFLAGS += -Ilua/

include $(TOPDIR)/build/build.prog.mk

download-here: lua/.git

.pc/done:
	@echo "[QUILT] apply patches $(DIR)"
	quilt push -a || [ $$? -ne 1 ]
	touch $@

build-before: .pc/done

install-here:
	@echo "[INSTALL] /lib/lua/init.lua"
	$(INSTALL) -D -m 644 init.lua $(SYSROOT)/lib/lua/init.lua

clean-here:
	@echo "[QUILT] remove patches $(DIR)"
	-test -f .pc/done && quilt pop -a
	rm -f .pc/done
