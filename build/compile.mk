ifndef SOURCES
$(error SOURCES is not set)
endif

SOURCES_C = $(filter %.c,$(SOURCES))
SOURCES_ASM = $(filter %.S,$(SOURCES))
OBJECTS += $(SOURCES_C:%.c=%.o) $(SOURCES_ASM:%.S=%.o)
DEPFILES = $(foreach f,$(SOURCES_C) $(SOURCES_ASM), \
	    $(dir $(f))$(patsubst %.c,.%.D,$(patsubst %.S,.%.D,$(notdir $(f)))))

define emit_dep_rule
CFILE = $(1)
DFILE = $(dir $(1))$(patsubst %.c,.%.D,$(patsubst %.S,.%.D,$(notdir $(1))))
$$(DFILE): $$(CFILE)
	@echo "[DEP] $$(DIR)$$@"
	$$(CC) $$(CFLAGS) $$(CPPFLAGS) -MM -MG $$^ -o $$@
endef

$(foreach file,$(SOURCES),$(eval $(call emit_dep_rule,$(file))))

build-dependencies: $(DEPFILES)

ifeq ($(words $(findstring $(MAKECMDGOALS), download clean distclean)), 0)
  -include $(DEPFILES)
endif

BUILD-FILES += $(OBJECTS)
CLEAN-FILES += $(DEPFILES) $(OBJECTS)
PRECIOUS-FILES += $(OBJECTS)
