include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding -G 0
CPPFLAGS += -I$(TOPDIR)/include -D_KERNEL
LDFLAGS  += -nostdlib

# KASAN
CFLAGS_NOKASAN := $(CFLAGS)
CFLAGS += -fsanitize=kernel-address --param asan-globals=1 --param asan-stack=1 -fsanitize-address-use-after-scope -fasan-shadow-offset=0xD8000000

boot.o : CFLAGS = $(CFLAGS_NOKASAN)
kasan.o : CFLAGS = $(CFLAGS_NOKASAN)
