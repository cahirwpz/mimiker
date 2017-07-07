# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

GENASSYM = $(TOPDIR)/script/genassym.py $(NM)

SOURCES_C = atomic.c \
	    cbus.c \
	    context.c \
	    cpu.c \
	    gt64120.c \
	    intr.c \
	    malta.c \
	    pmap.c \
	    rootdev.c \
	    stack.c \
	    signal.c \
	    timer.c \
	    tlb.c \
	    uart_cbus.c

SOURCES_ASM = boot.S copy.S exc.S switch.S ebase.S

all: $(DEPFILES) assym.h libmips.a

include $(TOPDIR)/build/build.kern.mk

$(SOURCES_ASM): assym.h

%.h: gen%.o
	@echo "[ASSYM] $(DIR)$@"
	$(GENASSYM) $< $@

libmips.a: $(OBJECTS)


clean:
	$(RM) .*.D *.ko *.o *.a *.lst *~
	$(RM) tags *.taghl
	$(RM) assym.h
