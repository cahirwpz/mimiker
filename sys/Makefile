# vim: tabstop=8 shiftwidth=8 noexpandtab:

SOURCES_C  = \
	assert.c \
	callout.c \
	clock.c \
	condvar.c \
	console.c \
	device.c \
	dev_cons.c \
	dev_null.c \
	dev_vga.c \
	devfs.c \
	drv_stdvga.c \
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
	pci.c \
	pci_ids.c \
	pcpu.c \
	physmem.c \
	pool.c \
	proc.c \
	runq.c \
	rwlock.c \
	sbrk.c \
	sched.c \
	signal.c \
	sleepq.c \
	startup.c \
	sync.c \
	sysent.c \
	sysinit.c \
	mmap.c \
	thread.c \
	uio.c \
	vfs.c \
	vfs_readdir.c \
	vfs_syscalls.c \
	vfs_vnode.c \
	vm_map.c \
	vm_object.c \
	vm_pager.c

SOURCES_ASM = 

all: $(DEPFILES) libsys.a 

include ../Makefile.common

libsys.a: $(OBJECTS)

clean:
	$(RM) .*.D *.ko *.o *.a *.lst *~
	$(RM) tags *.taghl
