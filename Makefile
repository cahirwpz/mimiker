# vim: tabstop=8 shiftwidth=8 noexpandtab:

all: cscope tags mimiker.elf initrd.cpio

# Disable all built-in recipes
.SUFFIXES:

include Makefile.common
$(info Using CC: $(CC))

# Directories which contain kernel parts
SYSSUBDIRS  = mips stdc sys tests
# Directories which require calling make recursively
SUBDIRS = $(SYSSUBDIRS) user

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: format tags cscope $(SUBDIRS) force

# Make sure the global cache dir exists before building user programs
user: | cache
cache:
	mkdir cache

mimiker.elf: $(KRT) | $(SYSSUBDIRS)
	@echo "[LD] Linking kernel image: $@"
	$(CC) $(LDFLAGS) -Wl,-Map=$@.map $(LDLIBS) -o $@

cscope:
	cscope -b include/*.h ./*/*.[cS]

tags:
	@echo "Rebuilding tags..."
	find -iname '*.[ch]' -not -path "*/toolchain/*" -and -not -path "*newlib*" | ctags --language-force=c -L-
	find -iname '*.[ch]' -not -path "*/toolchain/*" -and -not -path "*newlib*" | ctags --language-force=c -L- -e -f etags
	find -iname '*.S' -not -path "*/toolchain/*" -and -not -path "*newlib*" | ctags -a --language-force=asm -L-
	find -iname '*.S' -not -path "*/toolchain/*" -and -not -path "*newlib*" | ctags -a --language-force=asm -L- -e -f etags

# These files get destroyed by clang-format, so we exclude them from formatting
FORMATTABLE_EXCLUDE = include/elf stdc/ include/mips/asm.h include/mips/m32c0.h sysroot newlib-
# Search for all .c and .h files, excluding toolchain build directory and files from FORMATTABLE_EXCLUDE
FORMATTABLE = $(shell find -type f -not -path "*/toolchain/*" -and \( -name '*.c' -or -name '*.h' \) | grep -v $(FORMATTABLE_EXCLUDE:%=-e %))
format:
	@echo "Formatting files: $(FORMATTABLE:./%=%)"
	clang-format -style=file -i $(FORMATTABLE)

test: mimiker.elf
	./run_tests.py

# Detecting whether initrd.cpio requires rebuilding is tricky, because even if
# this target was to depend on $(shell fild sysroot -type f), then make compares
# sysroot files timestamps BEFORE recursively entering user and installing user
# programs into sysroot. This sounds silly, but apparently make assumes no files
# appear "without their explicit target". Thus, the only thing we can do is
# forcing make to always rebuild the archive.
initrd.cpio: force | user
	@echo "Building $@..."
	cd sysroot && find -depth -print | $(CPIO) -o -F ../$@ 2> /dev/null

clean:
	$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(DIR) $@;)
	$(RM) -f *.a *.elf *.map *.lst *~ *.log *.cpio .*.D
	$(RM) -f tags etags cscope.out *.taghl

distclean: clean
	$(RM) -rf cache sysroot

.PRECIOUS: %.uelf
