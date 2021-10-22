# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile which specifies aarch64 architecture specific settings.
#
# Required common variables: KERNEL, BOARD.
#

TARGET := aarch64-mimiker-elf
GCC_ABIFLAGS :=
CLANG_ABIFLAGS := -target $(TARGET)
ELFTYPE := elf64-littleaarch64
ELFARCH := aarch64

# NOTE(pj): We set A, SA, SA0 bits for SCTLR_EL1 register (sys/aarch64/boot.c)
# as a result not aligned access to memory causes an exception. It's okay but
# user-space (taken from NetBSD) doesn't care about that by default. It
# requies STRICT_ALIGNMENT to be defined. It's also important for the kernel
# because we share bcopy (memcpy) implementation with user-space.
CPPFLAGS += -DSTRICT_ALIGNMENT=1

ifeq ($(KERNEL), 1)
	CFLAGS += -mcpu=cortex-a53+nofp -march=armv8-a+nofp -mgeneral-regs-only
	ifeq ($(KASAN), 1)
	# Added to files that are sanitized
	CFLAGS_KASAN = -fsanitize=kernel-address \
		       -fasan-shadow-offset=0xe0001f0000000000 \
		       --param asan-globals=1 \
		       --param asan-stack=1 \
		       --param asan-instrument-allocas=1
	endif
else
	CFLAGS += -mcpu=cortex-a53 -march=armv8-a
endif

ifeq ($(BOARD), rpi3)
	KERNEL-IMAGES := mimiker.img mimiker.img.gz
endif
