# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

CFLAGS   = -ffreestanding
CPPFLAGS = -I$(TOPDIR)/include

SOURCES = $(wildcard src/*.c)

LIBFILES = libm.a

build: $(LIBFILES)

include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

$(LIBFILES): $(OBJECTS)

# Installation targets
LIBFILES_INST = $(LIBFILES:%=$(SYSROOT)/lib/%)

install-here: $(LIBFILES_INST)

$(LIBFILES_INST): $(LIBFILES)
	for LIBFILE in $(LIBFILES); do \
	  echo "[INSTALL] $(DIR)$$LIBFILE -> /lib/$$LIBFILE"; \
	  $(INSTALL) -m 644 $$LIBFILE $(SYSROOT)/lib/$$LIBFILE; \
	done

clean-here:
	$(RM) *.o *.a
