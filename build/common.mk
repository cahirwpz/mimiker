# vim: tabstop=8 shiftwidth=8 noexpandtab:

SYSROOT  = $(TOPDIR)/sysroot
CACHEDIR = $(TOPDIR)/cache
DIR = $(patsubst $(TOPDIR)/%,%,$(CURDIR)/)

# Pass "VERBOSE=1" at command line to display command being invoked by GNU Make
ifneq ($(VERBOSE), 1)
.SILENT:
endif

# Disable all built-in recipes
.SUFFIXES:

# Define our own recipes
%.S: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -S -o $@ $(realpath $<)

%.o: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(realpath $<)

%.o: %.S
	@echo "[AS] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $(realpath $<)

%.a:
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rs $@ $^ 2> /dev/null

# Generate recursive rules for subdirectories
define emit_subdir_rule
$(1)-$(2):
	@echo "[MAKE] $(2) $(DIR)$(1)"
	$(MAKE) -C $(1) $(2)
PHONY-TARGETS += $(1)-$(2)
endef

define emit_subdir_build
$(1)-build: $(1)-before
	@echo "[MAKE] build $(DIR)$(1)"
	$(MAKE) -C $(1) build
PHONY-TARGETS += $(1)-build $(1)-before
endef

$(foreach dir,$(SUBDIR),$(eval $(call emit_subdir_build,$(dir))))
$(foreach dir,$(SUBDIR),$(eval $(call emit_subdir_rule,$(dir),install)))
$(foreach dir,$(SUBDIR),$(eval $(call emit_subdir_rule,$(dir),clean)))
$(foreach dir,$(SUBDIR),$(eval $(call emit_subdir_rule,$(dir),distclean)))
$(foreach dir,$(SUBDIR),$(eval $(call emit_subdir_rule,$(dir),download)))

build-recursive: $(SUBDIR:%=%-build)
install-recursive: $(SUBDIR:%=%-install)
clean-recursive: $(SUBDIR:%=%-clean)
distclean-recursive: $(SUBDIR:%=%-distclean)
download-recursive: $(SUBDIR:%=%-download)

# Define main rules of the build system
download: download-recursive download-here
build: build-dependencies build-recursive $(BUILD-FILES) build-here
install: install-recursive $(INSTALL-FILES) install-here
clean: clean-recursive clean-here
	$(RM) $(CLEAN-FILES)
	$(RM) $(BUILD-FILES)
	$(RM) *~
distclean: clean distclean-recursive distclean-here

PHONY-TARGETS += all
PHONY-TARGETS += build build-dependencies build-recursive build-here
PHONY-TARGETS += clean clean-recursive clean-here
PHONY-TARGETS += install install-recursive install-here
PHONY-TARGETS += distclean distclean-recursive distclean-here
PHONY-TARGETS += download download-recursive download-here

.PHONY: $(PHONY-TARGETS)
.PRECIOUS: $(BUILD-FILES)

include $(TOPDIR)/build/tools.mk
