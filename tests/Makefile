TESTS = \
	callout.elf \
	crash.elf \
	exec.elf \
	exec_misbehave.elf \
	exec_fd_test.elf \
	linker_set.elf \
	malloc.elf \
	mutex.elf \
	physmem.elf \
	pmap.elf \
	rtc.elf \
	sched.elf \
	sleepq.elf \
	syscall.elf \
	thread.elf \
	uiomove.elf \
	vm_map.elf \
	vfs.elf
SOURCES_C = $(patsubst %.elf,%.c,$(TESTS))
SOURCES_ASM =

all: $(TESTS)

include ../Makefile.common

clean:
	$(RM) -f .*.D *.o *.elf *.map *~
