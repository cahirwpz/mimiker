# vim: tabstop=2 shiftwidth=2 noexpandtab:
#
# Common makefile which specifies MIPS architecture specific settings.
#
# Required common variables: KERNEL, BOARD.

TARGET := mipsel-mimiker-elf
# -G 0 disables small-data and small-bss,
# as otherwise they would exceed 64KB limit
GCC_ABIFLAGS := -mips32r2 -EL -G 0
CLANG_ABIFLAGS := -target $(TARGET) -march=mips32r2 -mno-abicalls \
		  -modd-spreg -G 0 -D__ELF__=1
ifeq ($(LLVM), 1)
  ELFTYPE := elf32-tradlittlemips
else
  ELFTYPE := elf32-littlemips
endif
ELFARCH := mips

ifeq ($(KERNEL), 1)
ifeq ($(KASAN), 1)
  # Added to files that are sanitized
  CFLAGS_KASAN = -fsanitize=kernel-address -fasan-shadow-offset=0xD8000000 \
                 --param asan-globals=1 \
                 --param asan-stack=1 \
                 --param asan-instrument-allocas=1
endif
# Added to all files
GCC_ABIFLAGS += -msoft-float
CLANG_ABIFLAGS += -msoft-float
ifeq ($(KGPROF), 1)
	CFLAGS_KGPROF = -finstrument-functions
endif
endif
