# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
TOPDIR = $(realpath ../..)

SOURCES = \
	main.c \
	broken.c \
	callout.c \
	crash.c \
	devfs.c \
	kmem.c \
	linker_set.c \
	mutex.c \
	physmem.c \
	pmap.c \
	pool.c \
	producer_consumer.c \
	resizable_fdt.c \
	ringbuf.c \
	sched.c \
	sleepq.c \
	sleepq_abort.c \
	sleepq_timed.c \
	strtol.c \
	thread_stats.c \
	thread_exit.c \
	turnstile_adjust.c \
	turnstile_propagate_once.c \
	turnstile_propagate_many.c \
	uiomove.c \
	vm_map.c \
	devclass.c \
	vfs.c \
	vmem.c

include $(TOPDIR)/build/build.kern.mk

CPPFLAGS += -DKL_LOG=KL_TEST
