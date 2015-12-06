# vim: tabstop=8 shiftwidth=8 noexpandtab:

P	= mips-mti-elf-
QEMU	= qemu-system-mipsel -machine pic32mz-wifire -sdl -serial stdio -s -S
CC	= $(P)gcc -mips32r2 -EL -g -nostdlib
AS	= $(P)as -mips32r2 -EL -g
GDB	= $(P)gdb
OBJCOPY	= $(P)objcopy
OBJDUMP	= $(P)objdump
CFLAGS	= -O -Wall -Werror -DPIC32MZ
LDSCRIPT= pic32mz.ld
LDFLAGS	= -T $(LDSCRIPT) -Wl,-Map=pic32mz.map

PROGNAME = main
SOURCES = main.c uart_raw.c
SOURCES_ASM = startup.s
OBJECTS := $(SOURCES:.c=.o)
OBJECTS_ASM := $(SOURCES_ASM:.s=.o)
OBJECTS += $(OBJECTS_ASM)

all: $(PROGNAME).srec

$(PROGNAME).srec: $(OBJECTS) $(LDSCRIPT)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(PROGNAME).elf
	$(OBJCOPY) -O srec $(PROGNAME).elf $(PROGNAME).srec

%.S: %.c
	$(AS) $(CFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

debug: $(PROGNAME).srec
	$(GDB) $(PROGNAME).elf

qemu: $(PROGNAME).srec
	$(QEMU) -kernel $(PROGNAME).srec

clean:
	rm -f *.o *.lst *~ *.elf *.srec *.map
