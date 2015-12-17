# vim: tabstop=8 shiftwidth=8 noexpandtab:

P	= mips-mti-elf-
QEMU	= qemu-system-mipsel -machine pic32mz-wifire -sdl -serial stdio -s -S
CC	= $(P)gcc -mips32r2 -EL -g -nostdlib
AS	= $(P)as  -mips32r2 -EL -g
GDB	= $(P)gdb
OBJCOPY	= $(P)objcopy
OBJDUMP	= $(P)objdump
CFLAGS	= -O -Wall -Werror -DPIC32MZ
LDSCRIPT= pic32mz.ld
LDFLAGS	= -T $(LDSCRIPT) -Wl,-Map=pic32mz.map

PROGNAME = main
SOURCES = main.c uart_raw.c
SOURCES_ASM = startup.S
OBJECTS := $(SOURCES:.c=.o)
OBJECTS_ASM := $(SOURCES_ASM:.S=.o)
OBJECTS += $(OBJECTS_ASM)

EXT_LIBS = smallclib/smallclib.a
INCLUDE = -isystemsmallclib/extra

all: $(PROGNAME).srec

$(PROGNAME).srec: $(OBJECTS) $(LDSCRIPT) smallclib
	$(CC) $(LDFLAGS) $(OBJECTS) $(EXT_LIBS) $(LIBS) -o $(PROGNAME).elf
	$(OBJCOPY) -O srec $(PROGNAME).elf $(PROGNAME).srec

%.S: %.c
	$(AS) $(CFLAGS) $(INCLUDE) -c $<

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

smallclib:
	$(MAKE) -C smallclib smallclib.a

debug: $(PROGNAME).srec
	$(GDB) $(PROGNAME).elf

qemu: $(PROGNAME).srec
	$(QEMU) -kernel $(PROGNAME).srec

clean:
	$(MAKE) -C smallclib clean
	rm -f *.o *.lst *~ *.elf *.srec *.map

.PHONY: smallclib
