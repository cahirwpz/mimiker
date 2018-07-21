# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

include $(TOPDIR)/build/build.mk
include $(TOPDIR)/build/flags.mk

CFLAGS   += -ffreestanding
CPPFLAGS += -I$(TOPDIR)/include

LIBFILE = libmimiker.a
EXTRAFILES = $(shell find extra -type f)

# Installation targets
LIBFILE_INST = $(SYSROOT)/usr/lib/$(LIBFILE)
EXTRAFILES_INST = $(EXTRAFILES:extra/%=$(SYSROOT)/%)

all: $(LIBFILE)
install: $(SYSROOT)/usr/lib/$(LIBFILE) $(EXTRAFILES_INST)

$(LIBFILE_INST): $(LIBFILE)
	@echo "[INSTALL] $(DIR)$(LIBFILE) -> /usr/lib/$(LIBFILE)"
	install -m 644 -D $(LIBFILE) $(LIBFILE_INST)
$(SYSROOT)/%: extra/%
	@echo "[INSTALL] $(DIR)$< -> $*"
	install -m 644 -D $< $@

libmimiker.a: gettimeofday.o nanosleep.o signal.o start.o syscalls.o

clean:
	$(RM) *.o *.a *~
