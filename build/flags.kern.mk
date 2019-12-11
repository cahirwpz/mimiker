include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding
CPPFLAGS += -I$(TOPDIR)/include -D_KERNEL
LDFLAGS  += -nostdlib

# KASAN
CFLAGS += -fsanitize=kernel-address
boot.o : CFLAGS += -fno-sanitize=kernel-address
kasan.o : CFLAGS += -fno-sanitize=kernel-address
