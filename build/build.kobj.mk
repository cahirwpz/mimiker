# vim: tabstop=8 shiftwidth=8 noexpandtab:

all: build

include $(TOPDIR)/build/flags.kern.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

%.ka: $(OBJECTS)
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rs $@ $^ 2> /dev/null

CLEAN-FILES += *.ka
