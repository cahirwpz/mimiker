# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(CURDIR)

all: cscope tags mimiker.elf initrd.cpio

include $(TOPDIR)/build/build.kern.mk

# Directories which contain kernel parts
SYSSUBDIRS  = mips stdc sys tests
# Directories which require calling make recursively
SUBDIRS = $(SYSSUBDIRS) user

$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: format tags cscope $(SUBDIRS) force

# Files required to link kernel image
KRT = $(TOPDIR)/stdc/libstdc.a \
      $(TOPDIR)/mips/libmips.a \
      $(TOPDIR)/sys/libsys.a \
      $(TOPDIR)/tests/libtests.a

# Process subdirectories before using KRT files.
$(KRT): | $(SYSSUBDIRS)
	true # Disable default recipe

LDFLAGS	= -nostdlib -T $(TOPDIR)/mips/malta.ld
LDLIBS	= -L$(TOPDIR)/sys -L$(TOPDIR)/mips -L$(TOPDIR)/stdc -L$(TOPDIR)/tests \
	  -Wl,--start-group \
	    -Wl,--whole-archive \
              -lsys \
	      -lmips \
              -ltests \
            -Wl,--no-whole-archive \
            -lstdc \
            -lgcc \
          -Wl,--end-group

mimiker.elf: $(KRT) | $(SYSSUBDIRS)
	@echo "[LD] Linking kernel image: $@"
	$(CC) $(LDFLAGS) -Wl,-Map=$@.map $(LDLIBS) -o $@

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
	@echo "[INITRD] Building $@..."
	cd sysroot && find -depth -print | $(CPIO) -o -F ../$@ 2> /dev/null

clean:
	$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(DIR) $@;)
	$(RM) *.a *.elf *.map *.lst *~ *.log *.cpio .*.D
	$(RM) tags etags cscope.out *.taghl

distclean: clean
	$(RM) -r cache sysroot
