# vim: tabstop=8 shiftwidth=8 noexpandtab:

include $(TOPDIR)/build/flags.kern.mk
include $(TOPDIR)/build/common.mk

build-kernel: $(KRT)
install-kernel: mimiker.elf

# Files required to link kernel image
KRT = $(foreach dir,$(KERNDIR),$(TOPDIR)/$(dir)/lib$(dir).ka)

LDFLAGS = -T $(TOPDIR)/mips/malta.ld
LDLIBS	= -Wl,--start-group \
	    -Wl,--whole-archive $(KRT) -Wl,--no-whole-archive \
            -lgcc \
          -Wl,--end-group

# Process subdirectories before using KRT files.
mimiker.elf: $(KRT) initrd.o dtb.o | $(KERNDIR)
	@echo "[LD] Linking kernel image: $@"
	$(CC) $(LDFLAGS) -Wl,-Map=$@.map $(LDLIBS) initrd.o dtb.o -o $@

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

initrd.o: initrd.cpio
	$(OBJCOPY) -I binary -O elf32-littlemips -B mips \
	  --rename-section .data=.initrd,alloc,load,readonly,data,contents \
	  $^ $@

# When multiple platforms are available, parametrize following rules.
malta.dtb:
	dtc -O dtb -o malta.dtb mips/malta.dts

dtb.o: malta.dtb
	@echo "[DTB] Building dtb.o from malta.dtb"
	$(OBJCOPY) -I binary -O elf32-littlemips -B mips \
	  --rename-section .data=.dtb,alloc,load,readonly,data,contents \
	  malta.dtb dtb.o


CLEAN-FILES += mimiker.elf mimiker.elf.map initrd.cpio initrd.o dtb.o malta.dtb
PHONY-TARGETS += initrd.cpio
