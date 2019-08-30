include $(TOPDIR)/build/flags.mk

CFLAGS   += -nostartfiles -nodefaultlibs --sysroot=$(SYSROOT)
LDFLAGS  += -nostartfiles -nodefaultlibs --sysroot=$(SYSROOT) -L= -T lib/ld.script
LDLIBS   +=
