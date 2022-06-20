# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile which specifies RISC-V architecture specific settings.
#
# Explanation of specified options can be found here:
#   https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Options.html#RISC-V-Options
#
# Required common variables: KERNEL, BOARD.
#

TARGET := riscv32-mimiker-elf
ELFTYPE := elf32-littleriscv
ELFARCH := riscv

ifeq ($(BOARD), litex-riscv)
	ifeq ($(CLANG), 1)
		EXT := ima
	else
		EXT := ima_zicsr_zifencei
	endif
	ABI := ilp32
	KERNEL_PHYS := 0x40000000
	KERNEL-IMAGES := mimiker.img
ifeq ($(KERNEL), 1)
	CPPFLAGS += -DFPU=0
endif
endif

GCC_ABIFLAGS += -march=rv32$(EXT) -mabi=$(ABI) 
CLANG_ABIFLAGS += -target riscv32-elf -march=rv32$(EXT) -mabi=$(ABI)

ifeq ($(KERNEL), 1)
	CFLAGS += -mcmodel=medany
	CPPFLAGS += -DKERNEL_PHYS=$(KERNEL_PHYS)
	CPPLDSCRIPT := 1
	ifeq ($(KASAN), 1)
	CFLAGS_KASAN = -fsanitize=kernel-address \
		       -fasan-shadow-offset=0x90000000 \
		       --param asan-globals=1 \
		       --param asan-stack=1 \
		       --param asan-instrument-allocas=1
	endif
endif
