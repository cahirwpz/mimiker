TARGET := aarch64-mimiker-elf
GCC_ABIFLAGS :=
CLANG_ABIFLAGS := -target aarch64-elf
ELFTYPE := elf64-littleaarch64
ELFARCH := aarch64

ifeq ($(KERNEL), 1)
CFLAGS += -mcpu=cortex-a53+nofp -march=armv8-a+nofp -mgeneral-regs-only
else
CFLAGS += -mcpu=cortex-a53 -march=armv8-a
endif

ifeq ($(BOARD), rpi3)
KERNEL-IMAGE := mimiker.img.gz
endif
