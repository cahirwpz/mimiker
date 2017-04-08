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
	producer_consumer.c \
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

all: $(DEPFILES) libtests.a test_initrd.cpio

test_initrd.cpio: build_test_initrd.sh
	./build_test_initrd.sh

include ../Makefile.common

libtests.a: $(OBJECTS)

clean:
	$(RM) -f .*.D *.o *.elf *.a *.map *~
