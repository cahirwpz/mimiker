# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

GENDEVHINT = $(TOPDIR)/sys/script/gendevhint.py

SOURCES = \
	boot.S \
	cbus.c \
	context.c \
	copy.S \
	cpu.c \
	ebase.S \
	exc.S \
	gt64120.c \
	intr.c \
	malta.c \
	pmap.c \
	rootdev.c \
	signal.c \
	switch.S \
	test-md.S \
	timer.c \
	tlb.c \
	uart_cbus.c \
	ustack.c

OBJECTS += malta_dtb.o

include $(TOPDIR)/build/build.kobj.mk

build-here: libmips.ka

$(SOURCES): assym.h

clean-here:
	$(RM) assym.h *-devhint.c
