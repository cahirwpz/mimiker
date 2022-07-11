# vim: tabstop=2 shiftwidth=2 noexpandtab:
#
# Common makefile which specifies aarch64 architecture specific settings.
#
# Required common variables: KERNEL, BOARD.
#

ELFTYPE := elf64-littleaarch64
ELFARCH := aarch64

ifeq ($(LLVM), 1)
  TARGET := aarch64-linux-mimiker-elf
  ABIFLAGS := -target $(TARGET)
else
  TARGET := aarch64-mimiker-elf
  ABIFLAGS := -mno-outline-atomics
endif

# NOTE(pj): We set A, SA, SA0 bits for SCTLR_EL1 register (sys/aarch64/boot.c)
# as a result not aligned access to memory causes an exception. It's okay but
# user-space (taken from NetBSD) doesn't care about that by default. It
# requies STRICT_ALIGNMENT to be defined. It's also important for the kernel
# because we share bcopy (memcpy) implementation with user-space.
CPPFLAGS += -DSTRICT_ALIGNMENT=1

ASAN_SHADOW_OFFSET := 0xe0001f0000000000 

ifeq ($(KERNEL), 1)
  CFLAGS += -mcpu=cortex-a53+nofp -march=armv8-a+nofp -mgeneral-regs-only
else
  CFLAGS += -mcpu=cortex-a53 -march=armv8-a
endif

ifeq ($(BOARD), rpi3)
  KERNEL-IMAGES := mimiker.img mimiker.img.gz
endif
