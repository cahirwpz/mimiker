# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile which specifies aarch64 architecture specific settings.
#
# Required common variables: KERNEL, BOARD.
#

TARGET := x86_64-mimiker-elf
GCC_ABIFLAGS :=
CLANG_ABIFLAGS := -target x86_64-elf
ELFTYPE := elf64-x86-64
ELFARCH := i386:x86-64
LDFLAGS += -z max-page-size=0x1000

ifeq ($(KERNEL), 1)
# Interrupts in kernel mode doesn't cause stack switch,
# hence leaf functions are in danger.
CFLAGS += -mno-red-zone 
# Avoid SIMD registers to increase performance.
CFLAGS += -mno-mmx -mno-sse -mno-avx
# Ensure absence of x87 registers.
CFLAGS += -msoft-float
endif
