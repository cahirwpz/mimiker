# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile used to supplement the compilation flags with
# the user space specific flags.
#
# Required common variables: SYSROOT.
#

include $(TOPDIR)/build/flags.mk

CPPFLAGS += -nostdinc --sysroot=$(SYSROOT) -I$(TOPDIR)/include
CFLAGS   += -ffreestanding -fno-builtin 
LDFLAGS  += --sysroot=$(SYSROOT) -L=/lib
LDLIBS   += -T $(SYSROOT)/lib/ld.script
