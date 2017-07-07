include $(TOPDIR)/build/flags.mk

GCC_INSTALL_PATH = $(shell LANG=C $(CC) -print-search-dirs | \
                     sed -n -e 's/install:\(.*\)/\1/p')
# The _LIBC_LIMITS_H_ prevents include-fixed/limits.h from forcefully including
# the system version of limits.h, which is not present in a freestanding
# environment. I feel like this is a workaround for a bug in GCC.
GCC_SYSTEM_INC = -I$(GCC_INSTALL_PATH)include/ \
                 -I$(GCC_INSTALL_PATH)include-fixed/ \
                 -D_LIBC_LIMITS_H_

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding
CPPFLAGS += -I$(TOPDIR)/include $(GCC_SYSTEM_INC) -D_KERNELSPACE 
