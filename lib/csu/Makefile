# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = $(ARCH)/crt0.S crt0-common.c
LDSCRIPT = $(ARCH)/ld.script

INSTALL-FILES = $(addprefix $(SYSROOT)/lib/, \
		  $(foreach f, $(OBJECTS) $(LDSCRIPT), $(notdir $(f))))

all: build

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

$(SYSROOT)/lib/%: %
	@echo "[INSTALL] $(DIR)$< -> /lib/$*"
	$(INSTALL) -m 644 $< $@

$(SYSROOT)/lib/%: $(ARCH)/%
	@echo "[INSTALL] $(DIR)$< -> /lib/$*"
	$(INSTALL) -m 644 $< $@
