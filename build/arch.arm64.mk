TARGET := aarch64-mimiker-elf
GCC_ABIFLAGS := -DELFSIZE=64
CLANG_ABIFLAGS := -target aarch64-elf -DELFSIZE=64
ELFTYPE := elf64-littleaarch64
ELFARCH := aarch64
