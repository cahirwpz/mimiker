# vim: tabstop=8 shiftwidth=8 noexpandtab:

P	= mips-mti-elf-
QEMU	= qemu-system-mipsel -machine pic32mz-wifire -sdl -serial stdio -s -S
CC	= $(P)gcc -mips32r2 -EL -g -nostdlib
GDB	= $(P)gdb
OBJCOPY	= $(P)objcopy
OBJDUMP	= $(P)objdump
CFLAGS	= -O -Wall -Werror -DPIC32MZ
LDFLAGS	= -T pic32mz.ld -e _start -Wl,-Map=pic32mz.map

PROG = uart

all: $(PROG).srec

$(PROG).srec: $(PROG).c
	$(CC) $(CFLAGS) -c $<
	$(CC) $(LDFLAGS) $(PROG).o $(LIBS) -o $(PROG).elf
	$(OBJCOPY) -O srec $(PROG).elf $(PROG).srec

debug: $(PROG).srec
	$(GDB) $(PROG).elf

qemu: $(PROG).srec
	$(QEMU) -kernel $(PROG).srec

clean:
	rm -f *.o *.lst *~ *.elf *.srec *.map
