# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile used throughout the mimiker build system.
# It defines basic mimiker specific recipes and the structure of the
# build system itself. 
#
# Characteristics of the build system:
# -The following standard targets are defined: download, install, build, 
#  clean, distclean, and format.
# -To accomplish any of the standard targets, the analogous target in all
#  the directories listed in SUBDIR must be accomplished first 
#  (using depth-first traversal). The only exception from this rule is the
#  format target, which will apply the recursion only if FORMAT-RECURSE
#  is undefined.
# -For each directory listed in SUBDIR, a special <subdir>-before target
#  may be supplied. If such a target exists, it will be executed before
#  executing the build target of the subdirectory.
# -The including makefile can define a <standard target>-here target which
#  will be executed after satisfying <standard target> for each of the
#  subdirectories.
# -Through the makefiles the TOPDIR variable is used. Whenever it occurs,
#  the including makefile should set it to the path to the mimiker directory
#  in the host system.
# -All make variables composing an API of a makefile from the build directory
#  must be set by the including makefile before the makefile is included.
#
# The following make variables are set by the including makefile:
# -VERBOSE: 1-recipes are loud, otherwise recipes are quiet.
# -SRCDIR: Source directory path relative to $(TOPDIR). This may be used
#  to build some sources outside the cwd (current working directory).
#  Defaults to cwd.
# -SUBDIR: Subdirectories to process.
# -DEPENDENCY-FILES: Files specifying dependencies (i.e. *.D files).
# -BUILD-FILES: Files to build at the current recursion level 
#  (besides build-here).
# -INSTALL-FILES: Files to install at the current recursion level
#  (besides install-here).
# -CLEAN-FILES: Files to remove at the current recursion level
#  (besides clean-here).
# -SOURCES_C: C sources to format at the current recursion level
#  (besides format-here).
# -SOURCES_H: C headers to format at the current recursion level
#  (besides format-here).
# -FORMAT_EXCLUDE: Files that shouldn't be formatted.
# -FORMAT-RECURSE: Should be set to "no" if recursion shouldn't be applied
#  to SUBDIR. Otherwise, must be undefined.
# -PHONY-TARGETS: Phony targets.
# For other variables see included makefiles.

SYSROOT  = $(TOPDIR)/sysroot
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
%.S: %.c
	@echo "[CC] $(SRCPATH) -> $(DSTPATH)"
	$(CC) $(CFLAGS) $(CFLAGS.$*.c) $(CPPFLAGS) $(WFLAGS) -S -o $@ \
	      $(realpath $<)

%.o: %.c
	@echo "[CC] $(SRCPATH) -> $(DSTPATH)"
	$(CC) $(CFLAGS) $(CFLAGS.$*.c) $(CFLAGS_KASAN) $(CPPFLAGS) $(WFLAGS) \
	      -c -o $@ $(realpath $<)

%.o: %.S
	@echo "[AS] $(SRCPATH) -> $(DSTPATH)"
	$(AS) $(ASFLAGS) $(CPPFLAGS) -c -o $@ $(realpath $<)

%.c: %.y
	@echo "[YACC] $(SCRPATH) -> $(DSTPATH)"
	$(YACC) -o $@ $(realpath $<)

%.a:
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DSTPATH)"
	$(AR) rs $@ $^ 2> /dev/null

assym.h: genassym.cf
	@echo "[ASSYM] $(DSTPATH)"
	$(GENASSYM) $(CC) $(ASSYM_CFLAGS) $(CFLAGS) $(CPPFLAGS) < $^ > $@

%/assym.h: %/genassym.cf
	@echo "[ASSYM] $(DSTPATH)"
	$(GENASSYM) $(CC) $(ASSYM_CFLAGS) $(CFLAGS) $(CPPFLAGS) < $^ > $@

include $(TOPDIR)/config.mk
include $(TOPDIR)/build/arch.$(ARCH).mk
include $(TOPDIR)/build/tools.mk

SRCDIR ?= .

vpath %.c $(SRCDIR)
vpath %.S $(SRCDIR)/$(ARCH)

# Recursive rules for subdirectories
%-format:
	@echo "[MAKE] format $(DIR)$*"
	$(MAKE) -C $* format

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
format-recursive: $(SUBDIR:%=%-format)

# Define main rules of the build system
download: download-recursive download-here
build: $(DEPENDENCY-FILES) build-recursive $(BUILD-FILES) build-here
install: install-recursive $(INSTALL-FILES) install-here
clean: clean-recursive clean-here
	$(RM) -v $(CLEAN-FILES)
	$(RM) -v $(BUILD-FILES)
	$(RM) -v *~
distclean: distclean-recursive distclean-here

FORMAT-FILES = $(filter-out $(FORMAT-EXCLUDE),$(SOURCES_C) $(SOURCES_H))
FORMAT-RECURSE ?= format-recursive

format: $(FORMAT-RECURSE) format-here
ifneq ($(FORMAT-FILES),)
	@echo "[FORMAT] $(FORMAT-FILES)"
	$(FORMAT) -i $(FORMAT-FILES)
endif

PHONY-TARGETS += all no
PHONY-TARGETS += build build-dependencies build-recursive build-here
PHONY-TARGETS += clean clean-recursive clean-here
PHONY-TARGETS += install install-recursive install-here
PHONY-TARGETS += distclean distclean-recursive distclean-here
PHONY-TARGETS += download download-recursive download-here
PHONY-TARGETS += format format-recursive format-here

.PHONY: $(PHONY-TARGETS)
.PRECIOUS: $(BUILD-FILES)
