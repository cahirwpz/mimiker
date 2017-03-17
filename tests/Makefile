SOURCES_C = \
	broken.c \
	callout.c \
	crash.c \
	exec.c \
	exec_misbehave.c \
	exec_fd_test.c \
	linker_set.c \
	malloc.c \
	mutex.c \
	physmem.c \
	pmap.c \
	producer_consumer.c \
	rtc.c \
	sched.c \
	strtol.c \
	syscall.c \
	thread.c \
	uiomove.c \
	vm_map.c \
	vfs.c
SOURCES_ASM =

all: $(DEPFILES) libtests.a

include ../Makefile.common

CFLAGS += -DCOMPILING_TESTS

libtests.a: $(OBJECTS)

clean:
	$(RM) -f .*.D *.o *.elf *.a *.map *~
