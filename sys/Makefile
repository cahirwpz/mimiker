# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C  = \
	assert.c \
	callout.c \
	clock.c \
	condvar.c \
	dev_null.c \
	dev_uart.c \
	devfs.c \
	exception.c \
	exec.c \
	file.c \
	filedesc.c \
	fork.c \
	interrupt.c \
	ktest.c \
	main.c \
	malloc.c \
	mutex.c \
	mmap.c \
	pci_ids.c \
	pcpu.c \
	physmem.c \
	runq.c \
	rwlock.c \
	sbrk.c \
	sched.c \
	sleepq.c \
	startup.c \
	sync.c \
	sysent.c \
	taskqueue.c \
	thread.c \
	uio.c \
	vfs.c \
	vfs_syscalls.c \
	vm_map.c \
	vm_object.c \
	vm_pager.c \
	vnode.c
SOURCES_ASM = 

all: $(DEPFILES) libsys.a 

include ../Makefile.common

libsys.a: $(OBJECTS)

clean:
	$(RM) -f .*.D *.ko *.o *.a *.lst *~
	$(RM) -f tags *.taghl
