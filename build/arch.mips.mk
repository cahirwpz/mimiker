# vim: tabstop=2 shiftwidth=2 noexpandtab:
#
# Common makefile which specifies MIPS architecture specific settings.
#
# Required common variables: KERNEL, BOARD.

ELFARCH := mips

# -G 0 disables small-data and small-bss,
# as otherwise they would exceed 64KB limit
ifeq ($(LLVM), 1)
  TARGET := mipsel-linux-mimiker-elf
  ABIFLAGS := -march=mips32r2 -mno-abicalls -modd-spreg -G 0 -D__ELF__=1
  ELFTYPE := elf32-tradlittlemips
else
  TARGET := mipsel-mimiker-elf
  ABIFLAGS := -mips32r2 -EL -G 0
  ELFTYPE := elf32-littlemips
endif

ASAN_SHADOW_OFFSET := 0xD8000000

ifeq ($(KERNEL), 1)
  ABIFLAGS += -msoft-float
endif
