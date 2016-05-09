# vim: tabstop=8 shiftwidth=8 noexpandtab:

include Makefile.common

OVPSIM_ROOT = ${IMPERAS_HOME}/lib/Linux32/ImperasLib/mips.ovpworld.org/platform/MipsMalta/1.0
OVPSIM   = ${OVPSIM_ROOT}/platform.Linux32.exe \
	   --port 1234 --nographics --wallclock \
	   --override mipsle1/vectoredinterrupt=1 \
	   --override mipsle1/srsctlHSS=1 \
	   --override rtc/timefromhost=1 \
	   --override uartCBUS/console=1

LDSCRIPT = malta.ld
LDFLAGS  += -T $(LDSCRIPT) -Wl,-Map=malta.map
CPPFLAGS += -Iinclude
LDLIBS   = smallclib/smallclib.a

PROGNAME = main
SOURCES_C = main.c uart_cbus.c interrupts.c clock.c malloc.c context.c \
	    context-demo.c vm_phys.c rtc.c pci.c pci_ids.c callout.c
SOURCES_ASM = startup.S intr.S context-mips.S mxxtlb_ops.S m32tlb_ops.S
SOURCES = $(SOURCES_C) $(SOURCES_ASM)
OBJECTS = $(SOURCES_C:.c=.o) $(SOURCES_ASM:.S=.o)
DEPFILES = $(SOURCES_C:%.c=.%.D) $(SOURCES_ASM:%.S=.%.D)

all: $(DEPFILES) $(PROGNAME).elf

$(PROGNAME).elf: smallclib $(OBJECTS) $(LDSCRIPT)

$(foreach file,$(SOURCES) null,$(eval $(call emit_dep_rule,$(file))))

ifeq ($(words $(findstring $(MAKECMDGOALS), clean)), 0)
  -include $(DEPFILES)
endif

smallclib:
	$(MAKE) -C smallclib smallclib.a

debug: $(PROGNAME).elf
	$(GDB) $<

sim: $(PROGNAME).elf
	$(OVPSIM) --kernel $(PROGNAME).elf --root /dev/null

astyle:
	astyle --options=astyle.options --recursive "*.h" "*.c" \
	       --exclude=include/bitset.h --exclude=include/_bitset.h \
	       --exclude=include/hash.h --exclude=include/queue.h \
	       --exclude=include/tree.h --exclude=vm_phys.c

clean:
	$(MAKE) -C smallclib clean
	$(RM) -f .*.D *.o *.lst *~ *.elf *.map *.log

.PHONY: smallclib qemu debug astyle
