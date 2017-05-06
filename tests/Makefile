SOURCES_C = \
	broken.c \
	callout.c \
	crash.c \
	exec.c \
	exec_misbehave.c \
	exec_fd_test.c \
	initrd.c \
	linker_set.c \
	malloc.c \
	mutex.c \
	physmem.c \
	pmap.c \
	pool.c \
	producer_consumer.c \
	resizable_fdt.c \
	rtc.c \
	rwlock.c \
	sched.c \
	strtol.c \
	syscall.c \
	thread_exit.c \
	uiomove.c \
	vm_map.c \
	vfs.c

SOURCES_ASM =

all: $(DEPFILES) initrd_test_files libtests.a

initrd_test_files: prepare_initrd_test_files.sh
	./prepare_initrd_test_files.sh

include ../Makefile.common

libtests.a: $(OBJECTS)

clean:
	$(RM) -f .*.D *.o *.elf *.a *.map *~
	$(RM) -rf initrd_test_files
