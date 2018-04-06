TOPDIR = $(realpath ..)
SYSROOT = $(TOPDIR)/sysroot

include $(TOPDIR)/build/build.mk

.PHONY: force

all: initrd.o

# Detecting whether initrd.cpio requires rebuilding is tricky, because even if
# this target was to depend on $(shell file sysroot -type f), then make compares
# sysroot files timestamps BEFORE recursively entering user and installing user
# programs into sysroot. This sounds silly, but apparently make assumes no files
# appear "without their explicit target". Thus, the only thing we can do is
# forcing make to always rebuild the archive.
initrd.cpio: force
	@echo "[INITRD] Building $@..."
	cd $(SYSROOT) && find -depth -print | $(CPIO) -o -F $(realpath .)/$@ 2> /dev/null

initrd.o: initrd.cpio
	$(OBJCOPY) -I binary -O elf32-littlemips -B mips \
		--rename-section .data=.initrd,alloc,load,readonly,data,contents \
		$^ $@

clean:
	$(RM) initrd.cpio initrd.o

