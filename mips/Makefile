# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C = clock.c context.c cpu.c interrupts.c pci.c physmem.c pmap.c tlb.c \
	    uart_cbus.c
SOURCES_ASM = boot.S switch.S tlb_ops.S

all: libmips.a intr.o

include ../Makefile.common

libmips.a: $(DEPFILES) $(OBJECTS)

clean:
	$(RM) -f .*.D *.ko *.o *.a *.lst *~
	$(RM) -f tags *.taghl
