# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

all: build

# Directories which require calling make recursively
SUBDIR = drv kern mips libkern tests

# Files required to link kernel image
KRT = $(foreach dir,$(SUBDIR),$(dir)/$(dir).ka)

LDFLAGS = -T $(TOPDIR)/sys/mips/malta.ld
LDLIBS	= -Wl,--start-group \
	    -Wl,--whole-archive $(KRT) -Wl,--no-whole-archive \
            -lgcc \
          -Wl,--end-group

mimiker.elf: $(KRT)
	@echo "[LD] Linking kernel image: $@"
	$(LD) $(LDFLAGS) -Wl,-Map=$@.map $(LDLIBS) -o $@

BUILD-FILES += mimiker.elf
CLEAN-FILES += mimiker.elf mimiker.elf.map

KLIB = no

include $(TOPDIR)/build/build.klib.mk
