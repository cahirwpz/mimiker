# vim: tabstop=8 shiftwidth=8 noexpandtab:

include Makefile.common

QEMU     = qemu-system-mipsel -machine pic32mz-wifire -sdl -serial stdio -s -S
LDSCRIPT = pic32mz.ld
LDFLAGS  += -T $(LDSCRIPT) -Wl,-Map=pic32mz.map
CPPFLAGS += -Iinclude
LDLIBS   = smallclib/smallclib.a

PROGNAME = main
SOURCES_C = main.c uart_raw.c interrupts.c clock.c bitmap.c
SOURCES_ASM = startup.S intr.S init_gpr.S init_cp0.S init_caches.S init_tlb.S
SOURCES = $(SOURCES_C) $(SOURCES_ASM)
OBJECTS = $(SOURCES_C:.c=.o) $(SOURCES_ASM:.S=.o)
DEPFILES = $(SOURCES_C:%.c=.%.D) $(SOURCES_ASM:%.S=.%.D)

all: $(DEPFILES) $(PROGNAME).srec

$(PROGNAME).elf: smallclib $(OBJECTS) $(LDSCRIPT)

$(foreach file,$(SOURCES) null,$(eval $(call emit_dep_rule,$(file))))

ifeq ($(words $(findstring $(MAKECMDGOALS), clean)), 0)
  -include $(DEPFILES)
endif

smallclib:
	$(MAKE) -C smallclib smallclib.a

debug: $(PROGNAME).srec
	$(GDB) $(PROGNAME).elf

qemu: $(PROGNAME).srec
	$(QEMU) -kernel $(PROGNAME).srec

clean:
	$(MAKE) -C smallclib clean
	$(RM) -f .*.D *.o *.lst *~ *.elf *.srec *.map

.PHONY: smallclib
