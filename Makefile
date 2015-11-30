# vim: tabstop=8 shiftwidth=8 noexpandtab:

P	= mips-mti-elf-
QEMU	= qemu-system-mipsel -machine pic32mz-wifire -sdl -serial stdio -s -S
CC	= $(P)gcc -mips32r2 -EL -g -nostdlib
GDB	= $(P)gdb
OBJCOPY	= $(P)objcopy
OBJDUMP	= $(P)objdump
CFLAGS	= -O -Wall -Werror -DPIC32MZ
LDFLAGS	= -T pic32mz.ld -e _start -Wl,-Map=pic32mz.map

PROGNAME = main
SOURCES = main.c uart_raw.c
OBJECTS := $(SOURCES:.c=.o)

all: $(PROGNAME).srec

$(PROGNAME).srec: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(PROGNAME).elf
	$(OBJCOPY) -O srec $(PROGNAME).elf $(PROGNAME).srec

%.o: %.c
	$(CC) $(CFLAGS) -c $<

debug: $(PROGNAME).srec
	$(GDB) $(PROGNAME).elf

qemu: $(PROGNAME).srec
	$(QEMU) -kernel $(PROGNAME).srec

clean:
	rm -f *.o *.lst *~ *.elf *.srec *.map
