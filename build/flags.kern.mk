include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding -fsanitize=kernel-address
boot.o : CFLAGS += -fno-sanitize=kernel-address
CPPFLAGS += -I$(TOPDIR)/include -D_KERNEL
LDFLAGS  += -nostdlib
