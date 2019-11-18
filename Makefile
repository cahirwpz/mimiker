# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(CURDIR)

# Directories which require calling make recursively
SUBDIR = sys lib bin usr.bin include

all: install

format: setup
build: setup
install: build
distclean: clean

bin-before: lib-install
usr.bin-before: lib-install

# Detecting whether initrd.cpio requires rebuilding is tricky, because even if
# this target was to depend on $(shell find sysroot -type f), then make compares
# sysroot files timestamps BEFORE recursively entering bin and installing user
# programs into sysroot. This sounds silly, but apparently make assumes no files
# appear "without their explicit target". Thus, the only thing we can do is
# forcing make to always rebuild the archive.
initrd.cpio: bin-install
	@echo "[INITRD] Building $@..."
	cd sysroot && \
	  find -depth \( ! -name "*.dbg" -and -print \) | sort | \
	    $(CPIO) -o -F ../$@ 2> /dev/null

INSTALL-FILES += initrd.cpio
CLEAN-FILES += initrd.cpio

distclean-here:
	$(RM) -r sysroot

setup:
	$(MAKE) -C include setup

test: sys-build initrd.cpio
	./run_tests.py

PHONY-TARGETS += setup test

include $(TOPDIR)/build/common.mk
