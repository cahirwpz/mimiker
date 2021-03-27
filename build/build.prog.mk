# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile for various simple usermode programs that reside in
# subdirectories. This is mostly a template for program-specific
# makefiles. Generally custom user programs will want to provide their own
# makefile, but for our own purposes this template is very convenient.
#
# The following make variables are set by the including makefile:
# - PROGRAM: The name for the resulting userspace ELF file. Generally, this will
#   be the program name installed into /bin directory.
#
# Following variables have default values assigned: SOURCES, BINDIR, BINMODE.
#

ifndef PROGRAM 
$(error PROGRAM is not set)
endif

SOURCES ?= $(PROGRAM).c
BINDIR ?= $(shell echo $(DIR) | cut -f 1 -d / | tr . /)

BUILD-FILES += $(PROGRAM).uelf
INSTALL-FILES += $(SYSROOT)/$(BINDIR)/$(PROGRAM)
BINMODE ?= 0755

all: build

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

# Linking the program according to the provided script
$(PROGRAM).uelf: $(OBJECTS)
	@echo "[LD] $(DIR)$< -> $(DIR)$@"
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

$(SYSROOT)/$(BINDIR)/$(PROGRAM): $(PROGRAM).uelf
	@echo "[INSTALL] $(DIR)$< -> /$(BINDIR)/$(PROGRAM)"
	$(INSTALL) -D $(PROGRAM).uelf --mode=$(BINMODE) \
		$(SYSROOT)/$(BINDIR)/$(PROGRAM)
	@echo "[OBJCOPY] $(SYSROOT)/$(BINDIR)/$(PROGRAM) -> " \
		"$(SYSROOT)/$(BINDIR)/$(PROGRAM).dbg"
	$(OBJCOPY) --only-keep-debug $(SYSROOT)/$(BINDIR)/$(PROGRAM) \
		$(SYSROOT)/$(BINDIR)/$(PROGRAM).dbg
	@echo "[STRIP] /$(BINDIR)/$(PROGRAM)"
	$(STRIP) --strip-all $(SYSROOT)/$(BINDIR)/$(PROGRAM)
