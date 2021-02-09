# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = \
	bus.c \
	callout.c \
	clock.c \
	cmdline.c \
	condvar.c \
	console.c \
	cred.c \
	cred_checks.c \
	cred_syscalls.c \
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
	fdt.c \
	file.c \
	file_syscalls.c \
	filedesc.c \
	fork.c \
	initrd.c \
	interrupt.c \
	isa.c \
	kenv.c \
	klog.c \
	kmem.c \
	ktest.c \
	main.c \
	malloc.c \
	mutex.c \
	mmap.c \
	pci.c \
	pci_ids.c \
	pcpu.c \
	pipe.c \
	pool.c \
	proc.c \
	pty.c \
	ringbuf.c \
	rman.c \
	runq.c \
	rwlock.c \
	sbrk.c \
	sched.c \
	signal.c \
	sleepq.c \
	spinlock.c \
	syscalls.c \
	taskqueue.c \
	turnstile.c \
	thread.c \
	time.c \
	timer.c \
	tmpfs.c \
	tty.c \
	uio.c \
	ustack.c \
	vfs.c \
	vfs_name.c \
	vfs_readdir.c \
	vfs_syscalls.c \
	vfs_vnode.c \
	vm_map.c \
	vm_object.c \
	vm_pager.c \
	vm_physmem.c \
	vmem.c

ifeq ($(KASAN), 1)
SOURCES += kasan.c kasan_quar.c
# KASAN's run-time library mustn't be sanitized!
kasan.o : CFLAGS_KASAN =
else
# Clean dependencies without having to pass `KASAN=1` to `make clean`.
CLEAN-FILES += .kasan.D .kasan_quar.D
endif

FORMAT-EXCLUDE = sysent.h

include $(TOPDIR)/build/build.kern.mk

SYSCALL_H = $(TOPDIR)/include/sys/syscall.h
SYSCALLARGS_H = $(TOPDIR)/include/sys/syscallargs.h

sysent.h $(SYSCALL_H) $(SYSCALLARGS_H): syscalls.conf syscalls.master
	@echo "[MAKESYSCALLS] $(lastword $^) -> $@"
	$(TOPDIR)/sys/script/makesyscalls.sh $^
