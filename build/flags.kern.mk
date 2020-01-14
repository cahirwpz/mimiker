include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding -G 0
CPPFLAGS += -I$(TOPDIR)/include -D_KERNEL
LDFLAGS  += -nostdlib

# KASAN
CFLAGS += -fsanitize=kernel-address # --param asan-stack=1

boot.o : CFLAGS = -std=gnu11 -Og -ggdb3 -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Werror -fno-builtin -nostdinc -nostdlib -ffreestanding 
kasan.o : CFLAGS = -std=gnu11 -Og -ggdb3 -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Werror -fno-builtin -nostdinc -nostdlib -ffreestanding 
