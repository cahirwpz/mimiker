# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile which specifies aarch64 architecture specific 
# settings.
#
# The following make variables are assumed to be set:
# -KERNEL: Defined if compiling a kernel source.
# -BOARD: Board defining the destination platform.

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
