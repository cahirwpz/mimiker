# vim: tabstop=2 shiftwidth=2 noexpandtab:
#
# Common makefile which specifies RISC-V architecture specific settings.
#
# Explanation of specified options can be found here:
#   https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Options.html#RISC-V-Options
#
# Required common variables: KERNEL, BOARD.
#

TARGET := riscv$(XLEN)-linux-mimiker-elf
ELFTYPE := elf$(XLEN)-littleriscv
ELFARCH := riscv

ifeq ($(BOARD), litex-riscv)
  ifeq ($(LLVM), 1)
    EXT := ima
  else
    EXT := ima_zicsr_zifencei
  endif
  ABI := ilp32
  KERNEL_PHYS := 0x40000000
  KERNEL_VIRT := 0x80000000
  KERNEL-IMAGES := mimiker.img
  ifeq ($(KERNEL), 1)
    CPPFLAGS += -DFPU=0
    ASAN_SHADOW_OFFSET := 0x90000000
  endif
endif

ifeq ($(BOARD), sifive_u)
  EXT := g
  ABI := lp64d
  KERNEL_PHYS := 0x80200000
  KERNEL_VIRT := 0xffffffc000000000
  ifeq ($(KERNEL), 1)
    CPPFLAGS += -DFPU=1
    ASAN_SHADOW_OFFSET := 0xdfffffe000000000
  endif
  ifeq ($(LLVM), 1)
    CPPFLAGS += -D__riscv_d -D__riscv_f
  endif
endif

GCC_ABIFLAGS += -march=rv$(XLEN)$(EXT) -mabi=$(ABI) 
CLANG_ABIFLAGS += -target $(TARGET) -march=rv$(XLEN)$(EXT) -mabi=$(ABI)

ifeq ($(KERNEL), 1)
  CFLAGS += -mcmodel=medany
  CPPFLAGS += -DKERNEL_PHYS=$(KERNEL_PHYS) -DKERNEL_VIRT=$(KERNEL_VIRT)
  CPPLDSCRIPT := 1
endif
