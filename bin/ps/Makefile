TOPDIR = $(realpath ../..)

PROGRAM = ps
BINMODE = 0755
INSTALL-FILES = $(SYSROOT)/$(BINDIR)/$(PROGRAM)
BINDIR = $(shell echo $(DIR) | cut -f 1 -d / | tr . /)

include $(TOPDIR)/build/common.mk

$(SYSROOT)/$(BINDIR)/$(PROGRAM): $(PROGRAM)
	@echo "[INSTALL] $(DIR)$< -> /$(BINDIR)/$(PROGRAM)"
	$(INSTALL) -D $(PROGRAM) --mode=$(BINMODE) $(SYSROOT)/$(BINDIR)/$(PROGRAM)
