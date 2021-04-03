TARGET := x86_64-mimiker-elf
GCC_ABIFLAGS :=
CLANG_ABIFLAGS := -target x86_64-elf
ELFTYPE := elf64-x86-64
ELFARCH := i386:x86-64
LDFLAGS += -z max-page-size=0x1000

ifeq ($(KERNEL), 1)
CFLAGS += -mno-red-zone
endif
