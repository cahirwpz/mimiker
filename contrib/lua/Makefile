TOPDIR = $(realpath ../..)

PKG = lua

PROGRAM = lua
BINDIR = bin
SRCDIR = $(PKG)
SOURCES = lua.c \
	  lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c \
	  lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c \
	  ltm.c lundump.c lvm.c lzio.c ltests.c \
	  lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c \
	  lstrlib.c lutf8lib.c lbitlib.c loadlib.c lcorolib.c lunix.c linit.c
FORMAT-EXCLUDE = $(SOURCES)

CPPFLAGS += -I$(PKG)/
BUILD-FILES += lua.uelf

all: download build install

include $(TOPDIR)/build/build.prog.mk

download-here: .pc/done

$(PKG)/.git:
	@echo "[GIT] submodule $(PKG)"
	$(GIT) submodule update --init $(PKG)

.pc/done: $(PKG)/.git
	@echo "[QUILT] apply patches"
	quilt push -a
	touch $@

install-here:
	@echo "[INSTALL] /lib/lua/init.lua"
	$(INSTALL) -D -m 644 init.lua $(SYSROOT)/lib/lua/init.lua

clean-here:
	@echo "[QUILT] remove patches"
	-test -f .pc/done && quilt pop -a
	rm -f .pc/done

