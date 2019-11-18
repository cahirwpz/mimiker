TARGET := mipsel-mimiker-elf
GCC_ABIFLAGS := -mips32r2 -EL -DELFSIZE=32
CLANG_ABIFLAGS := -target mipsel-elf -march=mips32r2 -mno-abicalls -modd-spreg -DELFSIZE=32
ELFTYPE := elf32-littlemips 
