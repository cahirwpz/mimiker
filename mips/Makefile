# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C = clock.c context.c cpu.c intr.c pci.c physmem.c pmap.c	\
	    stack.c sync.c tlb.c uart_cbus.c
SOURCES_ASM = boot.S exc.S switch.S syscall.S tlb_ops.S ebase.S

all: $(DEPFILES) genassym.h libmips.a

include ../Makefile.common

libmips.a: $(OBJECTS)

clean:
	$(RM) -f .*.D *.ko *.o *.a *.lst *~
	$(RM) -f tags *.taghl
	$(RM) -f genassym.h
