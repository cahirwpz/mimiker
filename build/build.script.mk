# Common makefile for installing userspace scripts.
#
# This template assumes following make variables are set:
#  SCRIPT: The name of script.

ifndef SCRIPT
$(error SCRIPT is not set)
endif

BINDIR ?= $(shell echo $(DIR) | cut -f 1 -d / | tr . /)

INSTALL-FILES += $(SYSROOT)/$(BINDIR)/$(SCRIPT)
BINMODE ?= 0755


include $(TOPDIR)/build/common.mk

$(SYSROOT)/$(BINDIR)/$(SCRIPT): $(SCRIPT)
	@echo "[INSTALL] $(DIR)$< -> /$(BINDIR)/$(SCRIPT)"
	$(INSTALL) -D $(SCRIPT) --mode=$(BINMODE) $(SYSROOT)/$(BINDIR)/$(SCRIPT)
