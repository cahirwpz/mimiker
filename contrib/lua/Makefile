TOPDIR = $(realpath ../..)

PROGRAM = lua
BINDIR = bin
SOURCES = $(wildcard lua/l*.c) lunix.c
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
