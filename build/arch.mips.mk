TARGET := mipsel-mimiker-elf
# -G 0 disables small-data and small-bss, as otherwise they would exceed 64KB limit
GCC_ABIFLAGS := -mips32r2 -EL -DELFSIZE=32 -G 0
CLANG_ABIFLAGS := -target mipsel-elf -march=mips32r2 -mno-abicalls -modd-spreg -DELFSIZE=32 -G 0
ELFTYPE := elf32-littlemips 

ifeq ($(KERNEL), 1)
KASAN ?= 0
ifeq ($(KASAN), 1)
  # Added to files that are sanitized
  CFLAGS_KASAN = -fsanitize=kernel-address -fasan-shadow-offset=0xD8000000 \
                 --param asan-globals=1 \
                 --param asan-stack=1 \
                 --param asan-instrument-allocas=1
endif
# Added to all files
CFLAGS += -DKASAN=$(KASAN)
endif
