# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C = atomic.c \
	    cbus.c \
	    clock.c \
	    context.c \
	    cpu.c \
	    gt64120.c \
	    intr.c \
	    malta.c \
	    pmap.c \
	    stack.c \
	    signal.c \
	    tlb.c \
	    uart_cbus.c

SOURCES_ASM = boot.S copy.S exc.S switch.S syscall.S tlb_ops.S ebase.S

all: $(DEPFILES) assym.h libmips.a

$(SOURCES_ASM): assym.h

include ../Makefile.common

libmips.a: $(OBJECTS)

clean:
	$(RM) .*.D *.ko *.o *.a *.lst *~
	$(RM) tags *.taghl
	$(RM) assym.h
