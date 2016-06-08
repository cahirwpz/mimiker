# vim: tabstop=8 shiftwidth=8 noexpandtab:

include Makefile.common

CPPFLAGS += -Iinclude
LDLIBS   += kernel.a smallclib/smallclib.a -lgcc

TESTS = rtc.elf runq.test
SOURCES_C = startup.c uart_cbus.c interrupts.c clock.c malloc.c context.c \
	    context-demo.c vm_phys.c rtc.c pci.c pci_ids.c callout.c runq.c
SOURCES_ASM = boot.S intr.S context-mips.S mxxtlb_ops.S m32tlb_ops.S
SOURCES = $(SOURCES_C) $(SOURCES_ASM)
OBJECTS = $(SOURCES_C:.c=.o) $(SOURCES_ASM:.S=.o)
DEPFILES = $(SOURCES_C:%.c=.%.D) $(SOURCES_ASM:%.S=.%.D)

all: $(DEPFILES) smallclib $(TESTS)

rtc.elf: rtc.ko kernel.a $(LDSCRIPT)
kernel.a: $(OBJECTS)

$(foreach file,$(SOURCES) null,$(eval $(call emit_dep_rule,$(file))))

ifeq ($(words $(findstring $(MAKECMDGOALS), clean)), 0)
  -include $(DEPFILES)
endif

smallclib:
	$(MAKE) -C smallclib smallclib.a

astyle:
	astyle --options=astyle.options --recursive "*.h" "*.c" \
	       --exclude=include/bitset.h --exclude=include/_bitset.h \
	       --exclude=include/hash.h --exclude=include/queue.h \
	       --exclude=include/tree.h --exclude=vm_phys.c

clean:
	$(MAKE) -C smallclib clean
	$(RM) -f .*.D *.ko *.o *.lst *~ *.elf *.map *.log
	$(RM) -f $(TESTS)

.PHONY: smallclib astyle
