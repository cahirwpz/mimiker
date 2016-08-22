# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C = callout.c interrupts.c malloc.c pci_ids.c physmem.c runq.c sched.c \
	    startup.c thread.c vm_map.c vm_object.c vm_pager.c
SOURCES_ASM = 

all: libsys.a 

include ../Makefile.common

libsys.a: $(DEPFILES) $(OBJECTS)

clean:
	$(RM) -f .*.D *.ko *.o *.a *.lst *~
	$(RM) -f tags *.taghl
