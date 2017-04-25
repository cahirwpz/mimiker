# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C  = \
	assert.c \
	callout.c \
	clock.c \
	condvar.c \
	console.c \
	dev_cons.c \
	dev_null.c \
	dev_vga.c \
	devfs.c \
	exception.c \
	exec.c \
	file.c \
	filedesc.c \
	fork.c \
	initrd.c \
	interrupt.c \
	klog.c \
	ktest.c \
	main.c \
	malloc.c \
	mutex.c \
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
	mmap.c \
	thread.c \
	uio.c \
	vga.c \
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
