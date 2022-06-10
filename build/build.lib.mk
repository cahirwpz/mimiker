# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile for user space libraries.
#

LIBNAME = $(shell basename $(CURDIR)).a
BUILD-FILES += $(LIBNAME)
LIBINST = $(LIBNAME:%=$(SYSROOT)/lib/%)
INSTALL-FILES += $(LIBINST)

all: build

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

$(LIBNAME): $(OBJECTS)
	@echo "[AR] $(addprefix $(DIR),$(OBJECTS)) -> $(DIR)$@"
	(echo "create $@"; \
	 for f in $(OBJECTS); do echo "addmod $$f"; done; \
	 echo "save"; \
	 echo "end") | $(AR) -M

$(LIBINST): $(LIBNAME)
	@echo "[INSTALL] $(DIR)$< -> /lib/$<"
	$(INSTALL) -m 644 $< $@
