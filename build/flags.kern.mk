include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding -finstrument-functions -finstrument-functions-exclude-file-list=sys/mips,sys/kern/prof.c,lib,include/sys -finstrument-functions-exclude-function-list=intr_disable,intr_enable,thread_self
CPPFLAGS += -I$(TOPDIR)/include -D_KERNEL
LDFLAGS  += -nostdlib

KERNEL := 1
