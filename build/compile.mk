# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile used to obtain objects and dependency files to build.
#
# Required common variables: SOURCES.
#

SOURCES_YACC = $(filter %.y,$(SOURCES))
SOURCES_GEN += $(SOURCES_YACC:%.y=%.c)

SOURCES_C = $(filter %.c,$(SOURCES)) $(SOURCES_GEN)
SOURCES_ALL_C = $(filter %.c,$(SOURCES_ALL)) $(SOURCES_GEN)
SOURCES_ASM = $(filter %.S,$(SOURCES))

OBJECTS += $(SOURCES_C:%.c=$(BUILDDIR)%.o) $(SOURCES_ASM:%.S=$(BUILDDIR)%.o)

DEPENDENCY-FILES += $(foreach f, $(SOURCES_C),\
		      $(dir $(f))$(patsubst %.c,%.d,$(notdir $(f))))
DEPENDENCY-FILES += $(foreach f, $(SOURCES_ASM),\
	  	      $(dir $(f))$(patsubst %.S,%.d,$(notdir $(f))))

$(DEPENDENCY-FILES): $(SOURCES_GEN)

$(BUILDDIR)%.d: %.c
	@echo "[DEP] $(DIR)$@"
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MT $*.o -MM -MG $^ -MF $@

$(BUILDDIR)%.d: %.S
	@echo "[DEP] $(DIR)$@"
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) $(CPPFLAGS) -MT $*.o -MM -MG $^ -MF $@

ifeq ($(words $(findstring $(MAKECMDGOALS), download clean distclean)), 0)
  -include $(DEPENDENCY-FILES)
endif

BUILD-FILES += $(OBJECTS)
CLEAN-FILES += $(DEPENDENCY-FILES) $(OBJECTS) $(SOURCES_GEN)
PRECIOUS-FILES += $(OBJECTS)
