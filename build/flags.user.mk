include $(TOPDIR)/build/flags.mk

CFLAGS   += --sysroot=$(SYSROOT)
LDFLAGS  += --sysroot=$(SYSROOT) -L= -T mimiker.ld
