# This is a common makefile for various simple usermode programs that reside in
# subdirectories. This is mostly a template for program-specific
# makefiles. Generally custom user programs will want to provide their own
# makefile, but for our own purposes this template is very convenient.

# This template assumes following make variables are set:
#  PROGRAM: The name for the resulting userspace ELF file. Generally, this will
#  be the program name installed into /bin directory.
#  SOURCES: C or assembly files to compile. Can be omitted if there's only one
#  file to compile named $(PROGRAM).c

ifndef PROGRAM 
$(error PROGRAM is not set)
endif

SOURCES ?= $(PROGRAM).c

BUILD-FILES += $(PROGRAM).uelf
INSTALL-FILES += $(SYSROOT)/bin/$(PROGRAM)

all: build

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

# Linking the program according to the provided script
$(PROGRAM).uelf: $(OBJECTS)
	@echo "[LD] $(DIR)$< -> $(DIR)$@"
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

$(SYSROOT)/bin/$(PROGRAM): $(PROGRAM).uelf
	@echo "[INSTALL] $(DIR)$< -> /bin/$(PROGRAM)"
	$(INSTALL) -D $(PROGRAM).uelf $(SYSROOT)/bin/$(PROGRAM)
	@echo "[OBJCOPY] $(SYSROOT)/bin/$(PROGRAM) -> \
		$(SYSROOT)/bin/$(PROGRAM).dbg"
	$(OBJCOPY) --only-keep-debug $(SYSROOT)/bin/$(PROGRAM) \
		$(SYSROOT)/bin/$(PROGRAM).dbg
	@echo "[STRIP] /bin/$(PROGRAM)"
	$(STRIP) --strip-all $(SYSROOT)/bin/$(PROGRAM)
