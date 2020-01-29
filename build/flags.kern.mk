include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding -G 0
CPPFLAGS += -I$(TOPDIR)/include -D_KERNEL
LDFLAGS  += -nostdlib

KERNEL := 1
