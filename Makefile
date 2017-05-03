# vim: tabstop=8 shiftwidth=8 noexpandtab:

all: cscope tags mimiker.elf initrd.cpio

# Disable all built-in recipes
.SUFFIXES:

include Makefile.common
$(info Using CC: $(CC))

# Newlib config
CACHEDIR = $(TOPDIR)/cache
NEWLIB = newlib-2.5.0
NEWLIBFILE = $(NEWLIB).tar.gz
NEWLIBDIR = $(CACHEDIR)/$(NEWLIBDIR)
NEWLIB_DOWNLOADS = ftp://sourceware.org/pub/newlib

# Directories which contain kernel parts
SYSSUBDIRS  = mips stdc sys tests
# Directories which require calling make recursively
SUBDIRS = $(SYSSUBDIRS) user libmimiker

$(SUBDIRS):
	$(MAKE) -C $@

# Make sure to have sysroot base and libmimiker prepared before compiling user programs
user: sysroot

.PHONY: format tags cscope $(SUBDIRS)

# This rule ensures that all subdirectories are processed before any file
# generated within them is used for linking the main kernel image.
$(KRT): | $(SYSSUBDIRS)
	true # Disable default recipe from Makefile.common

mimiker.elf: $(KRT)
	@echo "[LD] Linking kernel image: $@"
	$(CC) $(LDFLAGS) -Wl,-Map=$@.map $(LDLIBS) -o $@

cscope:
	cscope -b include/*.h ./*/*.[cS]

tags:
	@echo "Rebuilding tags"
	find -iname '*.[ch]' -not -path "*/toolchain/*" -and -not -path "cache/*" | ctags --language-force=c -L-
	find -iname '*.[ch]' -not -path "*/toolchain/*" -and -not -path "cache/*" | ctags --language-force=c -L- -e -f etags
	find -iname '*.S' -not -path "*/toolchain/*" -and -not -path "cache/*" | ctags -a --language-force=asm -L-
	find -iname '*.S' -not -path "*/toolchain/*" -and -not -path "cache/*" | ctags -a --language-force=asm -L- -e -f etags

# These files get destroyed by clang-format, so we exclude them from formatting
FORMATTABLE_EXCLUDE = include/elf stdc/ include/mips/asm.h include/mips/m32c0.h cache sysroot sysroot-newlib
# Search for all .c and .h files, excluding toolchain build directory and files from FORMATTABLE_EXCLUDE
FORMATTABLE = $(shell find -type f -not -path "*/toolchain/*" -and \( -name '*.c' -or -name '*.h' \) | grep -v $(FORMATTABLE_EXCLUDE:%=-e %))
format:
	@echo "Formatting files: $(FORMATTABLE:./%=%)"
	clang-format -style=file -i $(FORMATTABLE)

test: mimiker.elf
	./run_tests.py

initrd.cpio: | user sysroot tests
	./update_initrd.sh
# Import dependecies for initrd.cpio
include $(wildcard .*.D)

sysroot: sysroot-extra sysroot-newlib libmimiker/libmimiker.a
	@echo "Rebuilding sysroot..."
	rm -rf sysroot && mkdir sysroot
	# Install newlib into sysroot
	cp -r sysroot-newlib/ sysroot/usr
	# Instal libmimiker into sysroot
	cp libmimiker/libmimiker.a sysroot/usr/lib/.
	# Instal additional files into sysroot
	cp -rT sysroot-extra/ sysroot/

libmimiker/libmimiker.a: libmimiker
	$(MAKE) -C libmimiker libmimiker.a

sysroot-newlib: $(CACHEDIR)/$(NEWLIB)
	# Output from these commands is redirected to /dev/null for readibility. If
	# any of these commands fail, just remove output redirection.
	rm -rf sysroot-newlib && mkdir -p sysroot-newlib
	@echo "Configuring newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib; \
		CC=$(TRIPLET)-gcc ./configure --build=$(TRIPLET) --prefix=$(TOPDIR)/sysroot-newlib > /dev/null
	@echo "Compiling newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib; $(MAKE) > /dev/null 2>&1
	@echo "Installing newlib to sysroot-newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib; $(MAKE) install > /dev/null 2>&1

# The .tar.gz archive must be an order-only prerequisite, because the archive
# has a more recent last-modified date than the source directory.
$(CACHEDIR)/$(NEWLIB): | $(CACHEDIR)/$(NEWLIBFILE)
	@echo "Unpacking newlib..."
	tar -xzf $(CACHEDIR)/$(NEWLIBFILE) -C $(CACHEDIR)

$(CACHEDIR)/$(NEWLIBFILE):
	@echo "Downloading $(NEWLIBFILE). Don't worry, the download is cached so" \
		"there will be no need to do this again."
	mkdir -p $(CACHEDIR)
	wget $(NEWLIB_DOWNLOADS)/$(NEWLIBFILE) -O $@ || \
		(ret=$$?; rm -rf $@; exit $$ret) # Remove incomplete newlib archive on a failed download


clean:
	$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(DIR) $@;)
	$(RM) -f *.a *.elf *.map *.lst *~ *.log *.cpio .*.D
	$(RM) -f tags etags cscope.out *.taghl

distclean: clean
	$(RM) -rf cache sysroot-newlib

.PRECIOUS: %.uelf
