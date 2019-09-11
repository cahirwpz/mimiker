include $(TOPDIR)/build/flags.mk

CPPFLAGS += -nostdinc --sysroot=$(SYSROOT) -I$(SYSROOT)/usr/include
CFLAGS   += -fno-builtin 
LDFLAGS  += -nostartfiles -nodefaultlibs --sysroot=$(SYSROOT) -L= -T lib/ld.script
LDLIBS   +=
