# vim: tabstop=8 shiftwidth=8 noexpandtab:

TESTS = callout.elf malloc.elf physmem.elf pmap.elf rtc.elf sched.elf \
	thread.elf vm_map.elf
SOURCES_C = 
SOURCES_ASM = 

all: tags cscope mips stdc sys $(TESTS)

include Makefile.common

LDLIBS += -Lsys -Lmips -Lstdc \
	  -Wl,--start-group -lsys -lmips -lstdc -lgcc -Wl,--end-group

# Kernel runtime files
KRT = stdc mips sys

callout.elf: callout.ko $(KRT)
thread.elf: thread.ko $(KRT)
malloc.elf: malloc.ko $(KRT)
pmap.elf: pmap.ko $(KRT)
rtc.elf: rtc.ko $(KRT)
context.elf: context.ko $(KRT)
vm_map.elf: vm_map.ko $(KRT)
physmem.elf: physmem.ko $(KRT)
sched.elf: sched.ko $(KRT)

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

astyle:
	astyle --options=astyle.options --recursive "*.h" "*.c" \
	       --exclude=include/bitset.h --exclude=include/_bitset.h \
	       --exclude=include/hash.h --exclude=include/queue.h \
	       --exclude=include/tree.h --exclude=vm_phys.c

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

clean:
	$(MAKE) -C mips clean
	$(MAKE) -C stdc clean
	$(MAKE) -C sys clean
	$(RM) -f .*.D *.ko *.o *.a *.lst *~ *.elf *.map *.log
	$(RM) -f tags cscope.out *.taghl
	$(RM) -f $(TESTS)

.PHONY: astyle tags cscope mips stdc sys
