# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(CURDIR)

all: cscope tags mimiker.elf

include $(TOPDIR)/build/build.kern.mk

# Directories which contain kernel parts
SYSSUBDIRS  = mips stdc sys tests drv
# Directories which require calling make recursively
SUBDIRS = $(SYSSUBDIRS) user

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: format tags cscope $(SUBDIRS) force distclean download

# Files required to link kernel image
KRT = $(TOPDIR)/stdc/libstdc.a \
      $(TOPDIR)/mips/libmips.a \
      $(TOPDIR)/sys/libsys.a \
      $(TOPDIR)/drv/libdrv.a \
      $(TOPDIR)/tests/libtests.a

# Process subdirectories before using KRT files.
$(KRT): | $(SYSSUBDIRS)
	true # Disable default recipe

LDFLAGS	= -nostdlib -T $(TOPDIR)/mips/malta.ld
LDLIBS	= -L$(TOPDIR)/sys -L$(TOPDIR)/mips -L$(TOPDIR)/drv -L$(TOPDIR)/stdc -L$(TOPDIR)/tests \
	  -Wl,--start-group \
	    -Wl,--whole-archive \
              -lsys \
	      -ldrv \
	      -lmips \
              -lstdc \
              -ltests \
            -Wl,--no-whole-archive \
            -lgcc \
          -Wl,--end-group

mimiker.elf: $(KRT) initrd.o | $(SYSSUBDIRS)
	@echo "[LD] Linking kernel image: $@"
	$(CC) $(LDFLAGS) -Wl,-Map=$@.map $(LDLIBS) initrd.o -o $@

cscope:
	cscope -b include/*.h ./*/*.[cS]

# Lists of all files that we consider our sources.
SOURCE_RULES = -not -path "./toolchain/*" -and \
               -not -path "./user/newlib/newlib-*" -and \
               -not -path "./sysroot*"
SOURCES_C = $(shell find -iname '*.[ch]' -type f $(SOURCE_RULES))
SOURCES_ASM = $(shell find -iname '*.[S]' -type f $(SOURCE_RULES))

tags:
	@echo "[CTAGS] Rebuilding tags..."
	ctags --language-force=c $(SOURCES_C)
	ctags --language-force=c -e -f etags $(SOURCES_C)
	ctags --language-force=asm -a $(SOURCES_ASM)
	ctags --language-force=asm -aef etags $(SOURCES_ASM)

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
	clang-format-6.0 -style=file -i $(FORMATTABLE)

test: mimiker.elf
	./run_tests.py

# Detecting whether initrd.cpio requires rebuilding is tricky, because even if
# this target was to depend on $(shell find sysroot -type f), then make compares
# sysroot files timestamps BEFORE recursively entering user and installing user
# programs into sysroot. This sounds silly, but apparently make assumes no files
# appear "without their explicit target". Thus, the only thing we can do is
# forcing make to always rebuild the archive.
initrd.cpio: force
	make -C user install
	@echo "[INITRD] Building $@..."
	cd sysroot && find -depth -print | $(CPIO) -o -F ../$@ 2> /dev/null

initrd.o: initrd.cpio
	$(OBJCOPY) -I binary -O elf32-littlemips -B mips \
	  --rename-section .data=.initrd,alloc,load,readonly,data,contents \
	  $^ $@

clean:
	$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(DIR) $@;)
	$(RM) *.a *.elf *.map *.lst *~ *.log .*.D
	$(RM) tags etags cscope.out *.taghl
	$(RM) initrd.o initrd.cpio

distclean: clean
	$(MAKE) -C user/newlib distclean
	$(RM) -r cache sysroot

download:
	$(MAKE) -C user/newlib download
