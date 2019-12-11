# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = \
	boot.c \
	cbus.c \
	context.c \
	copy.S \
	cpu.c \
	ebase.S \
	gt64120.c \
	intr.c \
	$(PLATFORM).c \
	pmap.c \
	rootdev.c \
	sigcode.S \
	signal.c \
	start.S \
	switch.S \
	test-md.S \
	thread.c \
	timer.c \
	tlb.c \
	uart_cbus.c

OBJECTS += $(PLATFORM)_dtb.o

CLEAN-FILES += assym.h *-devhint.c

include $(TOPDIR)/build/build.kern.mk

$(SOURCES): assym.h
