SOURCES_C = $(filter %.c,$(SOURCES))
SOURCES_ASM = $(filter %.S,$(SOURCES))
OBJECTS += $(SOURCES_C:%.c=%.o) $(SOURCES_ASM:%.S=%.o)
DEPFILES = $(foreach f,$(SOURCES_C) $(SOURCES_ASM), \
	    $(dir $(f))$(patsubst %.c,.%.D,$(patsubst %.S,.%.D,$(notdir $(f)))))

define emit_dep_rule_c
CFILE = $(1)
DFILE = $(dir $(1))$(patsubst %.c,.%.D,$(notdir $(1)))
$$(DFILE): $$(CFILE)
	@echo "[DEP] $$(DIR)$$@"
	$$(CC) $$(CFLAGS) $$(CPPFLAGS) -MM -MG $$^ -o $$@
endef

define emit_dep_rule_asm
CFILE = $(1)
DFILE = $(dir $(1))$(patsubst %.S,.%.D,$(notdir $(1)))
$$(DFILE): $$(CFILE)
	@echo "[DEP] $$(DIR)$$@"
	$$(AS) $$(ASFLAGS) $$(CPPFLAGS) -MM -MG $$^ -o $$@
endef

$(foreach file,$(SOURCES_C),$(eval $(call emit_dep_rule_c,$(file))))
$(foreach file,$(SOURCES_ASM),$(eval $(call emit_dep_rule_asm,$(file))))

build-dependencies: $(DEPFILES)

ifeq ($(words $(findstring $(MAKECMDGOALS), download clean distclean)), 0)
  -include $(DEPFILES)
endif

BUILD-FILES += $(OBJECTS)
CLEAN-FILES += $(DEPFILES) $(OBJECTS)
PRECIOUS-FILES += $(OBJECTS)
