# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C = callout.c clock.c exception.c exec.c interrupt.c malloc.c sysent.c \
	    pci_ids.c physmem.c runq.c sched.c sleepq.c startup.c pci_ids.c \
	    thread.c turnstile.c vm_map.c vm_object.c vm_pager.c pcpu.c mutex.c
SOURCES_ASM = 

all: $(DEPFILES) libsys.a 

include ../Makefile.common

libsys.a: $(OBJECTS)

clean:
	$(RM) -f .*.D *.ko *.o *.a *.lst *~
	$(RM) -f tags *.taghl
