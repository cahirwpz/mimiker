SOURCES_C = \
	broken.c \
	callout.c \
	crash.c \
	klog.c \
	linker_set.c \
	malloc.c \
	mutex.c \
	physmem.c \
	pmap.c \
	pool.c \
	producer_consumer.c \
	resizable_fdt.c \
	rwlock.c \
	sched.c \
	strtol.c \
	taskqueue.c \
	thread_stats.c \
	thread_exit.c \
	uiomove.c \
	utest.c \
	vm_map.c \
	vfs.c

SOURCES_ASM =

all: $(DEPFILES) libtests.a

include ../Makefile.common

libtests.a: $(OBJECTS)

clean:
	$(RM) .*.D *.o *.elf *.a *.map *~
