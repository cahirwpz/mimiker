# vim: tabstop=8 shiftwidth=8 noexpandtab:

TESTS = callout.elf malloc.elf physmem.elf pmap.elf rtc.elf sched.elf	\
	sleepq.elf syscall.elf thread.elf vm_map.elf exec.elf				\
	exec_syscall.elf
SOURCES_C = 
SOURCES_ASM = 

all: tags cscope mips stdc sys $(TESTS)

include Makefile.common

LDLIBS += -Lsys -Lmips -Lstdc \
	  -Wl,--start-group -lsys -lmips -lstdc -lgcc -Wl,--end-group

LD_EMBED = user/prog.uelf.o user/syscall_test.uelf.o

# Kernel runtime files
KRT = stdc mips sys user

callout.elf: callout.ko $(KRT)
thread.elf: thread.ko $(KRT)
malloc.elf: malloc.ko $(KRT)
pmap.elf: pmap.ko $(KRT)
rtc.elf: rtc.ko $(KRT)
context.elf: context.ko $(KRT)
vm_map.elf: vm_map.ko $(KRT)
physmem.elf: physmem.ko $(KRT)
sched.elf: sched.ko $(KRT)
sleepq.elf: sleepq.ko $(KRT)
exec.elf: exec.ko $(KRT)
exec_syscall.elf: exec_syscall.ko $(KRT)

libkernel.a: $(DEPFILES) $(OBJECTS)

cscope:
	cscope -b include/*.h ./*.[cS] 

tags:
	find -iname '*.[ch]' | ctags --language-force=c -L-
	find -iname '*.[ch]' | ctags --language-force=c -L- -e -f etags
	find -iname '*.S' | ctags -a --language-force=asm -L-
	find -iname '*.S' | ctags -a --language-force=asm -L- -e -f etags
	find $(SYSROOT)/mips-mti-elf/include -type f -iname 'mips*' \
		| ctags -a --language-force=c -L-
	find $(SYSROOT)/mips-mti-elf/include -type f -iname 'mips*' \
		| ctags -a --language-force=c -L- -e -f etags
	find $(SYSROOT)/lib/gcc/mips-mti-elf/*/include -type f -iname '*.h' \
		| ctags -a --language-force=c -L-
	find $(SYSROOT)/lib/gcc/mips-mti-elf/*/include -type f -iname '*.h' \
		| ctags -a --language-force=c -L- -e -f etags

FORMATTABLE_EXCLUDE = include/elf stdc/smallclib include/mips/asm.h
FORMATTABLE = $(shell find -type f -name '*.c' -or -name '*.h' | grep -v $(FORMATTABLE_EXCLUDE:%=-e %))
format:
	@echo "Formatting files: $(FORMATTABLE:./%=%)"
	clang-format -style=file -i $(FORMATTABLE)

test:
	for file in $(wildcard *.test); do		\
	  echo -n "Running $${file}... ";		\
	  if ./$${file}; then				\
	    echo "\033[32;1mPASSED\033[0m";		\
	  else						\
	    echo "\033[31;1mFAILED\033[0m";		\
	  fi						\
	done

mips:
	$(MAKE) -C mips

stdc:
	$(MAKE) -C stdc 

sys:
	$(MAKE) -C sys

user:
	$(MAKE) -C user

clean:
	$(MAKE) -C mips clean
	$(MAKE) -C stdc clean
	$(MAKE) -C sys clean
	$(MAKE) -C user clean
	$(RM) -f .*.D *.ko *.o *.a *.lst *~ *.elf *.map *.log
	$(RM) -f tags cscope.out *.taghl
	$(RM) -f $(TESTS)

.PHONY: format tags cscope mips stdc sys user
