# This is a common makefile for various simple usermode programs that reside in
# subdirectories. This is mostly a template for program-specific
# makefiles. Generally custom user programs will want to provide their own
# makefile, but for our own purposes this template is very convenient.

# This template assumes following make variables are set:
#  SOURCES_C: The C files to compile.
#  UELF_NAME: The name for the resulting uelf file. Generally, this will be the
#    program name. The user program will be compiled into UELF_NAME.uelf file.

SOURCES_O = $(SOURCES_C:%.c=%.o)

all: $(UELF_NAME).uelf

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/build.mk

clean:
	rm -rf $(UELF_NAME).uelf $(SOURCES_C:%.c=.%.D) $(SOURCES_O)

# Linking the program according to the provided script
%.uelf: $(SOURCES_O)
	@echo "[LD] $(DIR)$< -> $(DIR)$@"
	$(CC) $(LDFLAGS) -o $@ $(SOURCES_O)

install: $(SYSROOT)/bin/$(UELF_NAME)

$(SYSROOT)/bin/$(UELF_NAME): $(UELF_NAME).uelf
	@echo "[INSTALL] $(DIR)$< -> /bin/$(UELF_NAME)"
	install -D $(UELF_NAME).uelf $(SYSROOT)/bin/$(UELF_NAME)

.PRECIOUS: %.uelf
