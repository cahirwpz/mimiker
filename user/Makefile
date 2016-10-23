all: prog.uelf.o

include ../Makefile.common

CFLAGS   = -std=gnu11 -O0 -Wall -Werror -fno-builtin -ffreestanding
LDFLAGS  = -nostdlib -T mimiker.ld

# Compiling the program source
prog.o: prog.c
	@echo "[CC] $< -> $@"
	$(CC) $(CFLAGS) -c -o $@ $<

# Linking the program according to the provided script
prog.uelf: prog.o start.o mimiker.ld
	@echo "[LD] $< -> $@"
	$(CC) $(LDFLAGS) start.o -Wl,-Map=$@.map -o $@ $<

# Treating the liked program as binary data, placing it in .rodata
# section, and embedding into an object file
prog.uelf.o: prog.uelf Makefile
	@echo "[LD] $< -> $@"
	$(OBJCOPY) -I binary --rename-section .data=.rodata,contents,readonly,data -O elf32-little --alt-machine-code=8 $< $@


clean:
	rm -f *.o prog.uelf.o prog.uelf prog.o prog.uelf.map
