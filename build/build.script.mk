# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile for installing userspace scripts.
#
# The following make variables are set by the including makefile:
# - SCRIPT: The name of the script.
#
# Following variables have default values assigned: BINDIR, BINMODE.
#

ifndef SCRIPT
$(error SCRIPT is not set)
endif

BINDIR ?= $(shell echo $(DIR) | cut -f 1 -d / | tr . /)

INSTALL-FILES += $(SYSROOT)/$(BINDIR)/$(SCRIPT)
BINMODE ?= 0755

all: install

include $(TOPDIR)/build/common.mk

$(SYSROOT)/$(BINDIR)/$(SCRIPT): $(SCRIPT)
	@echo "[INSTALL] $(DIR)$< -> /$(BINDIR)/$(SCRIPT)"
	$(INSTALL) -D $(SCRIPT) --mode=$(BINMODE) $(SYSROOT)/$(BINDIR)/$(SCRIPT)
