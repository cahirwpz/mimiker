# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(CURDIR)

# Directories which contain kernel parts
KERNDIR = drv mips stdc sys tests

all: cscope tags mimiker.elf

include $(TOPDIR)/build/build.kern.mk

# Directories which require calling make recursively
SUBDIRS = $(KERNDIR) lib bin 

$(SUBDIRS):
	@$(MAKE) -C $@

.PHONY: format tags cscope $(SUBDIRS) force distclean download

# Lists of all files that we consider operating system sources.
SRCDIRS = include $(KERNDIR) lib/libmimiker 
SOURCES_C = $(shell find $(SRCDIRS) -iname '*.[ch]')
SOURCES_ASM = $(shell find $(SRCDIRS) -iname '*.S')
SOURCES = $(SOURCES_C) $(SOURCES_ASM)

cscope.out: $(SOURCES)
	@echo "[CSCOPE] Rebuilding database..."
	$(CSCOPE) $(SOURCES)

tags:
	@echo "[CTAGS] Rebuilding tags..."
	$(CTAGS) --language-force=c $(SOURCES_C)
	$(CTAGS) --language-force=c -e -f etags $(SOURCES_C)
	$(CTAGS) --language-force=asm -a $(SOURCES_ASM)
	$(CTAGS) --language-force=asm -aef etags $(SOURCES_ASM)

# These files get destroyed by clang-format, so we explicitly exclude them from
# being automatically formatted
FORMATTABLE_EXCLUDE = \
	./include/elf/% \
	./include/mips/asm.h \
	./include/mips/m32c0.h \
	./stdc/%
FORMATTABLE = $(filter-out $(FORMATTABLE_EXCLUDE),$(SOURCES_C))

format:
	@echo "Formatting files: $(FORMATTABLE:./%=%)"
	$(FORMAT) -i $(FORMATTABLE)

test: mimiker.elf
	./run_tests.py

clean:
	$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(DIR) $@;)
	$(RM) *.a *.elf *.map *.lst *~ *.log .*.D
	$(RM) tags etags cscope.out *.taghl
	$(RM) initrd.o initrd.cpio

distclean: clean
	$(MAKE) -C lib/newlib distclean
	$(RM) -r cache sysroot

download:
	$(MAKE) -C lib/newlib download
