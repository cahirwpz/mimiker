# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile which specifies RISC-V architecture specific settings.
#
# Required common variables: KERNEL, BOARD.
#

TARGET := riscv32-mimiker-elf
GCC_ABIFLAGS := 
CLANG_ABIFLAGS := -target riscv32-elf
ELFTYPE := elf32-littleriscv
ELFARCH := riscv

ifeq ($(BOARD), vexriscv)
	KERNEL_PHYS = 0xc0000000
	KERNEL-IMAGES = mimiker.img

	EMULATOR-BIN = $(TOPDIR)/sys/emulator.bin
	EMULATOR-URL = https://dl.antmicro.com/projects/renode/litex_vexriscv--emulator.bin-s_9028-796a4227b806997c6629462fdf0dcae73de06929

$(EMULATOR-BIN):
	$(CURL) -o $@ $(EMULATOR-URL)
endif

ifneq ($(EMULATOR-BIN),)
sys-download: $(EMULATOR-BIN)
CLEAN-FILES += $(EMULATOR-BIN)
endif

ifeq ($(KERNEL), 1)
	CFLAGS += -mcmodel=medany
	CPPFLAGS += -DKERNEL_PHYS=$(KERNEL_PHYS)
	CPPLDSCRIPT = 1
endif
