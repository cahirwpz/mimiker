# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = \
	82371AB.c \
	boot.c \
	cbus.c \
	context.c \
	copy.S \
	cpu.c \
	ebase.S \
	gt64120.c \
	interrupt.c \
	malta.c \
	pmap.c \
	rootdev.c \
	sigcode.S \
	signal.c \
	start.S \
	switch.S \
	thread.c \
	timer.c \
	tlb.c \
	trap.c \
	uart_cbus.c

OBJECTS += $(BOARD)_dtb.o

CLEAN-FILES += assym.h

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk

ifeq ($(ARCH), mips)
$(SOURCES): assym.h
endif

boot.o : CFLAGS_KASAN =
