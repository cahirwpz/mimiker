# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C = atomic.c \
	    cbus.c \
	    clock.c \
	    context.c \
	    cpu.c \
	    gt64120.c \
	    intr.c \
	    ioports.c \
	    malta.c \
	    pci.c \
	    pmap.c \
	    stack.c \
	    tlb.c \
	    uart_cbus.c

SOURCES_ASM = boot.S copy.S exc.S switch.S syscall.S tlb_ops.S ebase.S

all: $(DEPFILES) assym.h libmips.a

$(SOURCES_ASM): assym.h

include ../Makefile.common

libmips.a: $(OBJECTS)

clean:
	$(RM) -f .*.D *.ko *.o *.a *.lst *~
	$(RM) -f tags *.taghl
	$(RM) -f assym.h
