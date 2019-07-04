# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(CURDIR)

# Directories which require calling make recursively
SUBDIR = sys lib bin usr.bin

all: build install

build-here: cscope.out tags
bin-before: lib-install

install-here: initrd.cpio

clean-here:
	$(RM) tags etags cscope.out *.taghl

distclean-here:
	$(RM) -r sysroot

# Detecting whether initrd.cpio requires rebuilding is tricky, because even if
# this target was to depend on $(shell find sysroot -type f), then make compares
# sysroot files timestamps BEFORE recursively entering bin and installing user
# programs into sysroot. This sounds silly, but apparently make assumes no files
# appear "without their explicit target". Thus, the only thing we can do is
# forcing make to always rebuild the archive.
initrd.cpio: bin-install
	@echo "[INITRD] Building $@..."
	cd sysroot && \
	  find -depth -print | sort | $(CPIO) -o -F ../$@ 2> /dev/null

CLEAN-FILES += initrd.cpio

# Lists of all files that we consider operating system sources.
SRCDIRS = include sys lib/libc lib/libm
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
	include/mips/asm.h \
	include/mips/m32c0.h \
	sys/stdc/%
FORMATTABLE = $(filter-out $(FORMATTABLE_EXCLUDE),$(SRCFILES_C))

format:
	@echo "Formatting files: $(FORMATTABLE:./%=%)"
	$(FORMAT) -i $(FORMATTABLE)

test: mimiker.elf
	./run_tests.py

PHONY-TARGETS += format tags cscope test

include $(TOPDIR)/build/common.mk
