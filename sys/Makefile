# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

# Directories which require calling make recursively
SUBDIR = drv kern mips stdc tests

all: build

build-here: mimiker.elf

include $(TOPDIR)/build/flags.kern.mk
include $(TOPDIR)/build/common.mk

# Files required to link kernel image
KRT = $(foreach dir,$(SUBDIR),$(TOPDIR)/sys/$(dir)/lib$(dir).ka)

LDFLAGS = -T $(TOPDIR)/sys/mips/malta.ld
LDLIBS	= -Wl,--start-group \
	    -Wl,--whole-archive $(KRT) -Wl,--no-whole-archive \
            -lgcc \
          -Wl,--end-group

# Process subdirectories before using KRT files.
mimiker.elf: $(KRT) | $(KERNDIR)
	@echo "[LD] Linking kernel image: $@"
	$(LD) $(LDFLAGS) -Wl,-Map=$@.map $(LDLIBS) -o $@

CLEAN-FILES += mimiker.elf mimiker.elf.map
