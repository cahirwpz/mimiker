# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile used to supplement the compilation flags with
# the user space specific flags.

include $(TOPDIR)/build/flags.mk

CPPFLAGS += -nostdinc --sysroot=$(SYSROOT) -I$(TOPDIR)/include
CFLAGS   += -ffreestanding -fno-builtin 
LDFLAGS  += -nostartfiles -nodefaultlibs --sysroot=$(SYSROOT) \
	    -L= -T lib/ld.script
LDLIBS   +=
