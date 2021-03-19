# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile for user space libraries.
#
# See included makefiles for customizable variables. 

LIBNAME = $(shell basename $(CURDIR)).a
BUILD-FILES += $(LIBNAME)
LIBINST = $(LIBNAME:%=$(SYSROOT)/lib/%)
INSTALL-FILES += $(LIBINST)

all: build

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

LIBLIST = $(foreach dir, $(SUBDIR), $(dir)/$(dir).a)
LIBDEPS = $(foreach dir, $(SUBDIR), $(dir)-build)

$(LIBNAME): $(OBJECTS) $(LIBDEPS)
	@echo "[AR] $(addprefix $(DIR),$(OBJECTS) $(LIBLIST)) -> $(DIR)$@"
	(echo "create $@"; \
	 for f in $(LIBLIST); do echo "addlib $$f"; done; \
	 for f in $(OBJECTS); do echo "addmod $$f"; done; \
	 echo "save"; \
	 echo "end") | $(AR) -M

$(LIBINST): $(LIBNAME)
	@echo "[INSTALL] $(DIR)$< -> /lib/$<"
	$(INSTALL) -m 644 $< $@
