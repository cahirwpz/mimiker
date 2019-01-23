# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

SOURCES = \
	assert.c \
	bus.c \
	callout.c \
	clock.c \
	condvar.c \
	console.c \
	devclass.c \
	device.c \
	dev_cons.c \
	dev_null.c \
	dev_vga.c \
	devfs.c \
	exception.c \
	exec.c \
	exec_elf.c \
	exec_shebang.c \
	file.c \
	filedesc.c \
	fork.c \
	initrd.c \
	interrupt.c \
	kbss.c \
	klog.c \
	ktest.c \
	main.c \
	malloc.c \
	mutex.c \
	mmap.c \
	pci.c \
	pci_ids.c \
	pcpu.c \
	physmem.c \
	pipe.c \
	pool.c \
	proc.c \
	ringbuf.c \
	rman.c \
	runq.c \
	rwlock.c \
	sbrk.c \
	sched.c \
	signal.c \
	sleepq.c \
	spinlock.c \
	startup.c \
	sysent.c \
	sysinit.c \
	taskqueue.c \
	turnstile.c \
	thread.c \
	time.c \
	timer.c \
	uio.c \
	vfs.c \
	vfs_readdir.c \
	vfs_syscalls.c \
	vfs_vnode.c \
	vm_map.c \
	vm_object.c \
	vm_pager.c \
	fdt.c

include $(TOPDIR)/build/build.kobj.mk

build-here: libsys.ka
