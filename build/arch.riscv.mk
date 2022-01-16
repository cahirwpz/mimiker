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
GCC_ABIFLAGS := 
CLANG_ABIFLAGS := -target riscv32-elf
ELFTYPE := elf32-littleriscv
ELFARCH := riscv

ifeq ($(BOARD), vexriscv)
	KERNEL_PHYS := 0x40000000
	KERNEL-IMAGES := mimiker.img
	CPPFLAGS += -DFPU=1 -DAUTO_DA_MGMT
endif

ifeq ($(KERNEL), 1)
	CFLAGS += -mcmodel=medany
	CPPFLAGS += -DKERNEL_PHYS=$(KERNEL_PHYS)
	CPPLDSCRIPT := 1
endif
