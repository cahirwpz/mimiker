# vim: tabstop=8 shiftwidth=8 noexpandtab:

KLIB ?= $(shell basename $(CURDIR)).ka

all: build

ifneq ($(KLIB), no)
BUILD-FILES += $(KLIB)
endif

include $(TOPDIR)/build/flags.kern.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

define emit_klib_build
$(1)/$(1).ka: $(1)-build
	@echo "[MAKE] build $(DIR)$(1)"
	$$(MAKE) -C $(1) build
PHONY-TARGETS += $(1)-build
endef

$(foreach dir,$(SUBDIR),$(eval $(call emit_klib_build,$(dir))))

# Pass "CLANG=1" at command line to switch kernel compiler to Clang.
ifeq ($(CLANG), 1)
CC = clang -target mipsel-elf -march=mips32r2 -mno-abicalls -g
endif

$(KLIB): $(OBJECTS) $(foreach dir, $(SUBDIR), $(dir)/$(dir).ka)
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rcT $@ $^ 2> /dev/null

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

CLEAN-FILES += *_dtb.o *.dtb
