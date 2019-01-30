# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(CURDIR)

# Directories which contain kernel parts
KERNDIR = drv mips stdc sys tests
# Directories which require calling make recursively
SUBDIR = $(KERNDIR) lib bin usr.bin

all: build install

build-here: cscope.out tags build-kernel
bin-before: lib-install

install-here: install-kernel

clean-here:
	$(RM) tags etags cscope.out *.taghl

distclean-here:
	$(RM) -r sysroot

# Lists of all files that we consider operating system sources.
SRCDIRS = include $(KERNDIR) lib/libmimiker 
SRCFILES_C = $(shell find $(SRCDIRS) -iname '*.[ch]')
SRCFILES_ASM = $(shell find $(SRCDIRS) -iname '*.S')
SRCFILES = $(SRCFILES_C) $(SRCFILES_ASM)

cscope.out: $(SRCFILES)
	@echo "[CSCOPE] Rebuilding database..."
	$(CSCOPE) $(SRCFILES)

tags:
	@echo "[CTAGS] Rebuilding tags..."
	$(CTAGS) --language-force=c $(SRCFILES_C)
	$(CTAGS) --language-force=c -e -f etags $(SRCFILES_C)
	$(CTAGS) --language-force=asm -a $(SRCFILES_ASM)
	$(CTAGS) --language-force=asm -aef etags $(SRCFILES_ASM)

# These files get destroyed by clang-format, so we explicitly exclude them from
# being automatically formatted
FORMATTABLE_EXCLUDE = \
	include/elf/% \
	include/mips/asm.h \
	include/mips/m32c0.h \
	stdc/%
FORMATTABLE = $(filter-out $(FORMATTABLE_EXCLUDE),$(SRCFILES_C))

format:
	@echo "Formatting files: $(FORMATTABLE:./%=%)"
	$(FORMAT) -i $(FORMATTABLE)

test: mimiker.elf
	./run_tests.py

PHONY-TARGETS += format tags cscope test

include $(TOPDIR)/build/build.kern.mk
