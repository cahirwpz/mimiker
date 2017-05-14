# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C = atomic.c \
	    clock.c \
	    context.c \
	    cpu.c \
	    gt64120.c \
	    intr.c \
	    malta.c \
	    pmap.c \
	    rootdev.c \
	    stack.c \
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
