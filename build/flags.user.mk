include $(TOPDIR)/build/flags.mk

CPPFLAGS += -nostdinc --sysroot=$(SYSROOT) -I$(TOPDIR)/include
CFLAGS   += -ffreestanding -fno-builtin 
LDFLAGS  += -nostartfiles -nodefaultlibs --sysroot=$(SYSROOT) -L= -T lib/ld.script
LDLIBS   +=
