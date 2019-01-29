# vim: tabstop=8 shiftwidth=8 noexpandtab:

all: build

include $(TOPDIR)/build/flags.kern.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

%.ka: $(OBJECTS)
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rs $@ $^ 2> /dev/null

%.dtb: %.dts
	@echo "[DTB] $(DIR)$< -> $(DIR)$@"
	dtc -O dtb -o $@ $<

%_dtb.o: %.dtb
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O elf32-littlemips -B mips \
	  --redefine-sym _binary_$(@:%.o=%)_start=__$(@:%.o=%)_start \
	  --redefine-sym _binary_$(@:%.o=%)_end=__$(@:%.o=%)_end \
	  --redefine-sym _binary_$(@:%.o=%)_size=__$(@:%.o=%)_size \
	  $< $@

CLEAN-FILES += *_dtb.o *.dtb *.ka
