TARGET := mipsel-mimiker-elf
GCC_ABIFLAGS := -mips32r2 -EL -DELFSIZE=32
CLANG_ABIFLAGS := -target mipsel-elf -march=mips32r2 -mno-abicalls -modd-spreg -DELFSIZE=32
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
