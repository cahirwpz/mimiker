# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile used throughout the mimiker build system.  It defines basic
# mimiker specific recipes and the structure of the build system itself. 
#
# The following make variables are set by the including makefile:
# - SRCDIR: Source directory path relative to $(TOPDIR). This may be used
#   to build some sources outside the cwd (current working directory).
#   Defaults to cwd.

SYSROOT = $(TOPDIR)/sysroot
DIR = $(patsubst $(TOPDIR)/%,%,$(CURDIR)/)

# Pass "VERBOSE=1" at command line to display command being invoked by GNU Make
ifneq ($(VERBOSE), 1)
.SILENT:
endif

# Disable all built-in recipes
.SUFFIXES:

SRCPATH = $(subst $(TOPDIR)/,,$(realpath $<))
DSTPATH = $(DIR)$@

# Define our own recipes
$(BUILDDIR)%.S: %.c
	@echo "[CC] $(SRCPATH) ($(CFLAGS) $(CFLAGS.$*.c) $(CPPFLAGS))"
	$(CC) $(CFLAGS) $(CFLAGS.$*.c) $(CPPFLAGS) $(WFLAGS) -S -o $@ \
	      $(realpath $<)

$(BUILDDIR)%.o: %.c
	@echo "[CC] $(SRCPATH) ($(CFLAGS) $(CFLAGS.$*.c) $(CFLAGS_KASAN) $(CFLAGS_KCSAN) $(CFLAGS_KFI) $(CPPFLAGS))"
	$(CC) $(CFLAGS) $(CFLAGS.$*.c) $(CFLAGS_KASAN) $(CFLAGS_KCSAN) $(CFLAGS_KFI) $(CPPFLAGS) $(WFLAGS) \
	      -c -o $@ $(realpath $<)

$(BUILDDIR)%.o: %.S
	@echo "[AS] $(SRCPATH) -> $(DSTPATH)"
	$(AS) $(ASFLAGS) $(CPPFLAGS) -c -o $@ $(realpath $<)

$(BUILDDIR)%.c: %.y
	@echo "[YACC] $(SCRPATH) -> $(DSTPATH)"
	$(YACC) -o $@ $(realpath $<)

$(BUILDDIR)%.a:
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DSTPATH)"
	$(AR) rs $@ $^ 2> /dev/null

assym.h: genassym.cf
	@echo "[ASSYM] $(DSTPATH)"
	$(GENASSYM) $(CC) $(ASSYM_CFLAGS) $(CFLAGS) $(CPPFLAGS) < $^ > $@

%/assym.h: %/genassym.cf
	@echo "[ASSYM] $(DSTPATH)"
	$(GENASSYM) $(CC) $(ASSYM_CFLAGS) $(CFLAGS) $(CPPFLAGS) < $^ > $@

%.ld: %.ld.in
	@echo "[CPP] $(SRCPATH) -> $(DSTPATH)"
	$(CPP) $(CPPFLAGS) -I$(TOPDIR)/include -P -o $@ $<

include $(TOPDIR)/config.mk
include $(TOPDIR)/build/arch.$(ARCH).mk
include $(TOPDIR)/build/tools.mk

SRCDIR ?= .

vpath %.c $(SRCDIR) $(SRCDIR)/$(ARCH)
vpath %.S $(SRCDIR)/$(ARCH)

# Recursive rules for subdirectories
%-download:
	@echo "[MAKE] download $(DIR)$*"
	$(MAKE) -C $* download 

%-build: %-download %-before
	@echo "[MAKE] build $(DIR)$*"
	$(MAKE) -C $* build

%-install: %-build
	@echo "[MAKE] install $(DIR)$*"
	$(MAKE) -C $* install

%-clean:
	@echo "[MAKE] clean $(DIR)$*"
	$(MAKE) -C $* clean

%-distclean: %-clean
	@echo "[MAKE] distclean $(DIR)$*"
	$(MAKE) -C $* distclean

PHONY-TARGETS += $(SUBDIR:%=%-before)

download-recursive: $(SUBDIR:%=%-download)
build-recursive: $(SUBDIR:%=%-build)
install-recursive: $(SUBDIR:%=%-install)
clean-recursive: $(SUBDIR:%=%-clean)
distclean-recursive: $(SUBDIR:%=%-distclean)

# Define main rules of the build system
download: download-recursive download-here
build: build-before $(DEPENDENCY-FILES) build-recursive $(BUILD-FILES) build-here
install: install-recursive $(INSTALL-FILES) install-here
clean: clean-recursive clean-here
	$(RM) -v $(CLEAN-FILES)
	$(RM) -v -r $(CLEAN-DIRS)
	$(RM) -v $(BUILD-FILES)
	$(RM) -v *~
distclean: distclean-recursive distclean-here

PHONY-TARGETS += all no
PHONY-TARGETS += build build-before build-recursive build-here
PHONY-TARGETS += clean clean-recursive clean-here
PHONY-TARGETS += install install-recursive install-here
PHONY-TARGETS += distclean distclean-recursive distclean-here
PHONY-TARGETS += download download-recursive download-here

.PHONY: $(PHONY-TARGETS)
.PRECIOUS: $(BUILD-FILES)
