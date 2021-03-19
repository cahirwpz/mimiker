# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile for kernel files. It is essentialy used for
# building kernel static libraties (i.e. *.ka archives) but may also be used
# to build a bunch of unrelated kernel sources.
#
# The following make variables are assumed to be set: 
# -KLIB: "no" if kernel library shouldn't be built. Otherwise, the variable
#  must be undefined.
# For other variables see included makefiles.
#
# Note that *_dtb.o targets should be manually added to the OBJECTS variable
# in the including makefile.

KLIB ?= $(shell basename $(CURDIR)).ka

all: build

ifneq ($(KLIB), no)
BUILD-FILES += $(KLIB)
endif

include $(TOPDIR)/build/flags.kern.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

KLIBLIST = $(foreach dir, $(SUBDIR), $(dir)/$(dir).ka)
KLIBDEPS = $(foreach dir, $(SUBDIR), $(dir)-build)

$(KLIB): $(OBJECTS) $(KLIBDEPS)
	@echo "[AR] $(addprefix $(DIR),$(OBJECTS) $(KLIBLIST)) -> $(DIR)$@"
	(echo "create $@"; \
	 for f in $(KLIBLIST); do echo "addlib $$f"; done; \
	 for f in $(OBJECTS); do echo "addmod $$f"; done; \
	 echo "save"; \
	 echo "end") | $(AR) -M

%.dtb: %.dts
	@echo "[DTB] $(DIR)$< -> $(DIR)$@"
	dtc -O dtb -o $@ $<

%_dtb.o: %.dtb
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary -O $(ELFTYPE) -B $(ELFARCH) \
	  --redefine-sym _binary_$(@:%.o=%)_start=__$(@:%.o=%)_start \
	  --redefine-sym _binary_$(@:%.o=%)_end=__$(@:%.o=%)_end \
	  --redefine-sym _binary_$(@:%.o=%)_size=__$(@:%.o=%)_size \
	  $< $@

CLEAN-FILES += *_dtb.o *.dtb
