# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

CFLAGS   = -ffreestanding
CPPFLAGS = -I$(TOPDIR)/include

SOURCES = \
	gettimeofday.c \
	nanosleep.c \
	signal.c \
	start.S \
	syscalls.S

LIBFILE = libmimiker.a

all: $(LIBFILE)

include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

$(LIBFILE): $(OBJECTS)

EXTRAFILES = $(shell find extra -type f)

# Installation targets
LIBFILE_INST = $(SYSROOT)/usr/lib/$(LIBFILE)
EXTRAFILES_INST = $(EXTRAFILES:extra/%=$(SYSROOT)/%)

install-here: $(SYSROOT)/usr/lib/$(LIBFILE) $(EXTRAFILES_INST)

$(LIBFILE_INST): $(LIBFILE)
	@echo "[INSTALL] $(DIR)$(LIBFILE) -> /usr/lib/$(LIBFILE)"
	$(INSTALL) -m 644 $(LIBFILE) $(LIBFILE_INST)

$(SYSROOT)/%: extra/%
	@echo "[INSTALL] $(DIR)$< -> $*"
	$(INSTALL) -m 644 $< $@

clean-here:
	$(RM) *.o *.a
