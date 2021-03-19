# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile used to obtain objects and dependency files to
# build.
#
# The following make variables are assumed to be set:
# -SOURCES: The *.[cSy] files to compile.

SOURCES_YACC = $(filter %.y,$(SOURCES))
SOURCES_GEN += $(SOURCES_YACC:%.y=%.c)

SOURCES_C = $(filter %.c,$(SOURCES)) $(SOURCES_GEN)
SOURCES_ASM = $(filter %.S,$(SOURCES))

OBJECTS += $(SOURCES_C:%.c=%.o) $(SOURCES_ASM:%.S=%.o)

DEPENDENCY-FILES += $(foreach f, $(SOURCES_C),\
		      $(dir $(f))$(patsubst %.c,.%.D,$(notdir $(f))))
DEPENDENCY-FILES += $(foreach f, $(SOURCES_ASM),\
	  	      $(dir $(f))$(patsubst %.S,.%.D,$(notdir $(f))))

$(DEPENDENCY-FILES): $(SOURCES_GEN)

.%.D: %.c
	@echo "[DEP] $(DIR)$@"
	$(CC) $(CFLAGS) $(CPPFLAGS) -MT $*.o -MM -MG $^ -MF $@

.%.D: %.S
	@echo "[DEP] $(DIR)$@"
	$(AS) $(ASFLAGS) $(CPPFLAGS) -MT $*.o -MM -MG $^ -MF $@

ifeq ($(words $(findstring $(MAKECMDGOALS), download clean distclean)), 0)
  -include $(DEPENDENCY-FILES)
endif

BUILD-FILES += $(OBJECTS)
CLEAN-FILES += $(DEPENDENCY-FILES) $(OBJECTS) $(SOURCES_GEN)
PRECIOUS-FILES += $(OBJECTS)
