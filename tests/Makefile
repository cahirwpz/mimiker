TESTS = \
	callout.elf \
	crash.elf \
	exec.elf \
	exec_fd_test.elf \
	exec_misbehave.elf \
	initrd.elf \
	linker_set.elf \
	malloc.elf \
	mutex.elf \
	physmem.elf \
	pmap.elf \
	producer_consumer.elf \
	rtc.elf \
	sched.elf \
	syscall.elf \
	thread.elf \
	uiomove.elf \
	vfs.elf \
	vm_map.elf
SOURCES_C = $(patsubst %.elf,%.c,$(TESTS))
SOURCES_ASM =

all: $(TESTS)

include ../Makefile.common

clean:
	$(RM) -f .*.D *.o *.elf *.map *~
