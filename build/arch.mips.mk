TARGET := mipsel-mimiker-elf
ABIFLAGS := -mips32r2 -EL -DELFSIZE=32
ELFTYPE := elf32-littlemips 

# Pass "CLANG=1" at command line to switch kernel compiler to Clang.
ifeq ($(CLANG), 1)
CC = clang -target mipsel-elf -march=mips32r2 -mno-abicalls -g
endif
