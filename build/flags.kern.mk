include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding -G 0
CPPFLAGS += -I$(TOPDIR)/include -D_KERNEL
LDFLAGS  += -nostdlib

# KASAN
CFLAGS += -fsanitize=kernel-address --param asan-globals=1 -fsanitize-address-use-after-scope -fasan-shadow-offset=0xF0000000

boot.o : CFLAGS = -std=gnu11 -Og -ggdb3 -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Werror -fno-builtin -nostdinc -nostdlib -ffreestanding -G 0
kasan.o : CFLAGS = -std=gnu11 -Og -ggdb3 -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Werror -fno-builtin -nostdinc -nostdlib -ffreestanding -G 0
