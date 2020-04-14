TARGET := mipsel-mimiker-elf
# -G 0 disables small-data and small-bss, as otherwise they would exceed 64KB limit
GCC_ABIFLAGS := -mips32r2 -EL -DELFSIZE=32 -G 0
CLANG_ABIFLAGS := -target mipsel-elf -march=mips32r2 -mno-abicalls -modd-spreg -DELFSIZE=32 -G 0
ELFTYPE := elf32-littlemips 

# Set KASAN flags
ifeq ($(KASAN), 1)
ifeq ($(KERNEL), 1)
	# Added to files that are sanitized
	CFLAGS_KASAN = -fsanitize=kernel-address --param asan-globals=1 --param asan-stack=1 -fasan-shadow-offset=0xD8000000
	# Added to all files
	CFLAGS += -DKASAN
endif
endif
