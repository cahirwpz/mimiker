# vim: tabstop=8 shiftwidth=8 noexpandtab:

all: mimiker.elf tags cscope initrd.cpio

# Disable all built-in recipes
.SUFFIXES:

include Makefile.common
$(info Using CC: $(CC))

# Directories which require calling make recursively
SUBDIRS = mips stdc sys user tests

# This rule ensures that all subdirectories are processed before any file
# generated within them is used for linking the main kernel image.
$(KRT): | $(SUBDIRS)
	true # Disable default recipe from Makefile.common

mimiker.elf: $(KRT)
	@echo "[LD] Linking kernel image: $@"
	$(CC) $(LDFLAGS) -Wl,-Map=$@.map $(LDLIBS) -o $@

cscope:
	cscope -b include/*.h ./*/*.[cS]

tags:
	find -iname '*.[ch]' -not -path "*/toolchain/*" | ctags --language-force=c -L-
	find -iname '*.[ch]' -not -path "*/toolchain/*" | ctags --language-force=c -L- -e -f etags
	find -iname '*.S' -not -path "*/toolchain/*" | ctags -a --language-force=asm -L-
	find -iname '*.S' -not -path "*/toolchain/*" | ctags -a --language-force=asm -L- -e -f etags

# These files get destroyed by clang-format, so we exclude them from formatting
FORMATTABLE_EXCLUDE = include/elf stdc/ include/mips/asm.h include/mips/m32c0.h
# Search for all .c and .h files, excluding toolchain build directory and files from FORMATTABLE_EXCLUDE
FORMATTABLE = $(shell find -type f -not -path "*/toolchain/*" -and \( -name '*.c' -or -name '*.h' \) | grep -v $(FORMATTABLE_EXCLUDE:%=-e %))
format:
	@echo "Formatting files: $(FORMATTABLE:./%=%)"
	clang-format -style=file -i $(FORMATTABLE)

test: mimiker.elf
	./run_tests.py

$(SUBDIRS):
	$(MAKE) -C $@

initrd.cpio: | $(SUBDIRS)
	./update_initrd.sh
# Import dependecies for initrd.cpio
include $(wildcard .*.D)

clean:
	$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(DIR) $@;)
	$(RM) -f *.a *.elf *.map *.lst *~ *.log *.cpio .*.D
	$(RM) -f tags etags cscope.out *.taghl

.PHONY: format tags cscope $(SUBDIRS)
.PRECIOUS: %.uelf
