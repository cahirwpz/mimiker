ifndef SOURCES
$(error SOURCES is not set)
endif

SOURCES_C = $(filter %.c,$(SOURCES))
SOURCES_ASM = $(filter %.S,$(SOURCES))
OBJECTS = $(SOURCES_C:%.c=%.o) $(SOURCES_ASM:%.S=%.o)
DEPFILES = $(SOURCES_C:%.c=.%.D) $(SOURCES_ASM:%.S=.%.D)

define emit_dep_rule
CFILE = $(1)
DFILE = $(dirname $(1))/.$(patsubst %.S,%.D,$(patsubst %.c,%.D,$(basename $(1))))
$$(DFILE): $$(CFILE)
	@echo "[DEP] $$(DIR)$$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM -MG $$^ -o $$@
endef

$(foreach file,$(SOURCES) null,$(eval $(call emit_dep_rule,$(file))))

ifeq ($(words $(findstring $(MAKECMDGOALS), clean)), 0)
  -include $(DEPFILES)
endif

clean::
	$(RM) $(DEPFILES) $(OBJECTS)
	$(RM) *~
