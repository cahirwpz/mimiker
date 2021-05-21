# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile used to supplement the compilation flags with the kernel
# specific flags.
#

include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding
CPPFLAGS += -I$(TOPDIR)/include -D_KERNEL
CPPFLAGS += -DLOCKDEP=$(LOCKDEP) -DKASAN=$(KASAN) -DKGPROF=$(KGPROF) -DKCSAN=$(KCSAN)
LDFLAGS  += -nostdlib

KERNEL := 1
