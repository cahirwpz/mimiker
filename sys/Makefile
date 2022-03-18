# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

include $(TOPDIR)/config.mk

SUBDIR = aarch64 drv dts kern libfdt libkern mips riscv tests
KLIB = no

BUILD-FILES += $(KERNEL-IMAGES) mimiker.elf cscope.out etags tags
CLEAN-FILES += mimiker.elf.map

include $(TOPDIR)/build/build.kern.mk

LDSCRIPT = $(ARCH)/$(ARCH).ld
LDFLAGS += -T $(TOPDIR)/sys/$(LDSCRIPT)
LDLIBS	= -Wl,--start-group \
	    -Wl,--whole-archive $(KLIBLIST) -Wl,--no-whole-archive \
            -lgcc \
          -Wl,--end-group

mimiker.elf: $(KLIBDEPS) $(LDSCRIPT)
	@echo "[LD] Linking kernel image: $@"
	$(LD) $(LDFLAGS) -Wl,-Map=$@.map $(LDLIBS) -o $@

mimiker.img: mimiker.elf
	@echo "[OBJCOPY] Prepare kernel image: $@"
	$(OBJCOPY) -O binary $^ $@

mimiker.img.gz: mimiker.img
	@echo "[GZIP] Compress kernel image: $@"
	$(GZIP) -k -f $^ > $@

# Lists of all files that we consider operating system kernel sources.
SRCDIRS = $(TOPDIR)/include $(TOPDIR)/sys
SRCFILES_C = $(shell find $(SRCDIRS) -iname '*.[ch]')
SRCFILES_ASM = $(shell find $(SRCDIRS) -iname '*.S')
SRCFILES = $(SRCFILES_C) $(SRCFILES_ASM)

cscope.out: $(SRCFILES)
	@echo "[CSCOPE] Rebuilding cscope database..."
	$(CSCOPE) $(SRCFILES)

etags tags:
	@echo "[CTAGS] Rebuilding tags database..."
	$(CTAGS) --language-force=c $(SRCFILES_C)
	$(CTAGS) --language-force=c -e -f etags $(SRCFILES_C)
	$(CTAGS) --language-force=asm -a $(SRCFILES_ASM)
	$(CTAGS) --language-force=asm -aef etags $(SRCFILES_ASM)

BUILD-FILES += cscope.out etags tags
CLEAN-FILES += *.taghl
