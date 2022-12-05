TOPDIR = $(realpath ../..)

PROGRAM = lua
BINDIR = bin
SRCDIR = lua
SOURCES = lua.c \
	  lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c \
	  lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c \
	  ltm.c lundump.c lvm.c lzio.c ltests.c \
	  lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c \
	  lstrlib.c lutf8lib.c lbitlib.c loadlib.c lcorolib.c lunix.c linit.c
FORMAT-EXCLUDE = $(SOURCES)

CPPFLAGS += -Ilua/

all: download build

include $(TOPDIR)/build/build.prog.mk

download-here: .pc/done

lua/.git:
	@echo "[GIT] update submodule $(DIR)"
	$(GIT) submodule update --init lua

.pc/done: lua/.git
	@echo "[QUILT] apply patches $(DIR)"
	quilt push -a
	touch $@

install-here:
	@echo "[INSTALL] /lib/lua/init.lua"
	$(INSTALL) -D -m 644 init.lua $(SYSROOT)/lib/lua/init.lua

clean-here:
	@echo "[QUILT] remove patches $(DIR)"
	-test -f .pc/done && quilt pop -a
	rm -f .pc/done

