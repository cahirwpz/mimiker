include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding
CPPFLAGS += -I$(TOPDIR)/include -D_KERNEL
LDFLAGS  += -nostdlib

KERNEL := 1

KPROF ?= 0
ifeq ($(KPROF), 1)
  CFLAGS_KPROF = -finstrument-functions -finstrument-functions-exclude-file-list=sys/kern/prof.c,lib -finstrument-functions-exclude-function-list=intr_disable,intr_enable,thread_self,intr_root_handler
endif
# Added to all files
CFLAGS += -DKPROF=$(KPROF)