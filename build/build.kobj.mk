# vim: tabstop=8 shiftwidth=8 noexpandtab:

include $(TOPDIR)/build/flags.kern.mk
include $(TOPDIR)/build/build.mk

%.ka: $(OBJECTS)
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rs $@ $^ 2> /dev/null

clean::
	$(RM) *.ka
