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
	interrupt.c \
	$(BOARD).c \
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
	uart_cbus.c \
	ucontext.c

OBJECTS += $(BOARD)_dtb.o

CLEAN-FILES += assym.h *-devhint.c

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk

$(SOURCES): assym.h

boot.o : CFLAGS_KASAN =
