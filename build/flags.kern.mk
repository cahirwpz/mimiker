# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile used to supplement the compilation flags with the kernel
# specific flags.
#

include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding
CPPFLAGS += -I$(TOPDIR)/include -I$(TOPDIR)/sys/contrib -D_KERNEL
CPPFLAGS += -DLOCKDEP=$(LOCKDEP) -DKASAN=$(KASAN) -DKGPROF=$(KGPROF) -DKCSAN=$(KCSAN)
LDFLAGS  += -nostdlib

ifeq ($(KCSAN), 1)
  # Added to files that are sanitized
  CFLAGS_KCSAN = -fsanitize=thread \
                  --param tsan-distinguish-volatile=1
endif

KERNEL := 1
