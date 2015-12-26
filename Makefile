# vim: tabstop=8 shiftwidth=8 noexpandtab:

include Makefile.common

QEMU     = qemu-system-mipsel -machine pic32mz-wifire -sdl -serial stdio -s -S
LDSCRIPT = pic32mz.ld
LDFLAGS  += -T $(LDSCRIPT) -Wl,-Map=pic32mz.map
CPPFLAGS += -isystemsmallclib/include
LDLIBS   = smallclib/smallclib.a

PROGNAME = main
SOURCES_C = main.c uart_raw.c interrupts.c
SOURCES_ASM = startup.S intr.S
OBJECTS_C := $(SOURCES_C:.c=.o)
OBJECTS_ASM := $(SOURCES_ASM:.S=.o)
OBJECTS = $(OBJECTS_C) $(OBJECTS_ASM)

all: $(PROGNAME).srec

$(PROGNAME).elf: smallclib $(OBJECTS) $(LDSCRIPT)

smallclib:
	$(MAKE) -C smallclib smallclib.a

debug: $(PROGNAME).srec
	$(GDB) $(PROGNAME).elf

qemu: $(PROGNAME).srec
	$(QEMU) -kernel $(PROGNAME).srec

clean:
	$(MAKE) -C smallclib clean
	$(RM) -f *.o *.lst *~ *.elf *.srec *.map

.PHONY: smallclib
