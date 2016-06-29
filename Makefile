# vim: tabstop=8 shiftwidth=8 noexpandtab:

include Makefile.common

CPPFLAGS += -Iinclude
LDLIBS   += kernel.a -Llibkern -lkern -lgcc

TESTS = callout.elf context.elf malloc.elf pmap.elf pm.elf rtc.elf runq.test
SOURCES_C = startup.c uart_cbus.c interrupts.c clock.c malloc.c context.c \
	    pm.c rtc.c pci.c pci_ids.c callout.c runq.c tlb.c pmap.c
SOURCES_ASM = boot.S intr.S context-mips.S tlb-mips.S
SOURCES = $(SOURCES_C) $(SOURCES_ASM)
OBJECTS = $(SOURCES_C:.c=.o) $(SOURCES_ASM:.S=.o)
DEPFILES = $(SOURCES_C:%.c=.%.D) $(SOURCES_ASM:%.S=.%.D)

all: $(DEPFILES) tags cscope.out libkern $(TESTS)

callout.elf: callout.ko kernel.a
context.elf: context.ko kernel.a
malloc.elf: malloc.ko kernel.a
rtc.elf: rtc.ko kernel.a
kernel.a: $(OBJECTS)

$(foreach file,$(SOURCES) null,$(eval $(call emit_dep_rule,$(file))))

ifeq ($(words $(findstring $(MAKECMDGOALS), clean)), 0)
  -include $(DEPFILES)
endif

libkern:
	$(MAKE) -C libkern libkern.a

cscope.out:
	cscope -bv include/*.h ./*.[cS] 

tags:
	find -iname '*.h' | ctags -L- --c-kinds=+p

astyle:
	astyle --options=astyle.options --recursive "*.h" "*.c" \
	       --exclude=include/bitset.h --exclude=include/_bitset.h \
	       --exclude=include/hash.h --exclude=include/queue.h \
	       --exclude=include/tree.h --exclude=vm_phys.c

clean:
	$(MAKE) -C libkern clean
	$(RM) -f .*.D *.ko *.o *.a *.lst *~ *.elf *.map *.log
	$(RM) -f tags cscope.out
	$(RM) -f $(TESTS)

.PHONY: libkern astyle
