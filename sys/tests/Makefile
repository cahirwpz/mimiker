# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
TOPDIR = $(realpath ../..)

SOURCES = \
	broken.c \
	callout.c \
	crash.c \
	devfs.c \
	fdt.c \
	klog.c \
	kmem.c \
	linker_set.c \
	mutex.c \
	physmem.c \
	pmap.c \
	pool.c \
	producer_consumer.c \
	resizable_fdt.c \
	ringbuf.c \
	rwlock.c \
	sched.c \
	sleepq.c \
	sleepq_abort.c \
	sleepq_timed.c \
	strtol.c \
	taskqueue.c \
	thread_stats.c \
	thread_exit.c \
	time.c \
	turnstile_adjust.c \
	turnstile_propagate_once.c \
	turnstile_propagate_many.c \
	uiomove.c \
	utest.c \
	vm_map.c \
	devclass.c \
	vfs.c \
	vmem.c

OBJECTS += fake_dtb.o

include $(TOPDIR)/build/build.kern.mk
