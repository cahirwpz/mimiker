# vim: tabstop=2 shiftwidth=2 noexpandtab:
#
# Common makefile used to supplement the compilation flags with the kernel
# specific flags.
#

include $(TOPDIR)/build/flags.mk

CFLAGS   += -fno-builtin -nostdinc -nostdlib -ffreestanding
CPPFLAGS += -I$(TOPDIR)/include -I$(TOPDIR)/sys/contrib -D_KERNEL
CPPFLAGS += -DLOCKDEP=$(LOCKDEP) -DKASAN=$(KASAN) -DKGPROF=$(KGPROF) -DKCSAN=$(KCSAN)
LDFLAGS  += -nostdlib

ifeq ($(KCSAN), 1)
  # Added to files that are sanitized
  CFLAGS_KCSAN = -fsanitize=thread \
                  --param tsan-distinguish-volatile=1
endif

ifeq ($(KASAN), 1)
  # Added to files that are sanitized
  ifeq ($(LLVM), 1)
    CFLAGS_KASAN = -fsanitize=kernel-address \
                   -mllvm -asan-mapping-offset=$(ASAN_SHADOW_OFFSET) \
                   -mllvm -asan-instrumentation-with-call-threshold=0 \
                   -mllvm -asan-globals=true \
                   -mllvm -asan-stack=true \
                   -mllvm -asan-instrument-dynamic-allocas=true
  else
    CFLAGS_KASAN = -fsanitize=kernel-address \
                   -fasan-shadow-offset=$(ASAN_SHADOW_OFFSET) \
                   --param asan-instrumentation-with-call-threshold=0 \
                   --param asan-globals=1 \
                   --param asan-stack=1 \
                   --param asan-instrument-allocas=1
  endif
  LDFLAGS += -wrap=copyin -wrap=copyinstr -wrap=copyout \
             -wrap=bcopy -wrap=bzero -wrap=memcpy -wrap=memmove -wrap=memset
endif

ifeq ($(KGPROF), 1)
  CFLAGS_KGPROF = -finstrument-functions
endif

KERNEL := 1
