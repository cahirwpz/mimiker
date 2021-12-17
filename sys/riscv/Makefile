# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES-RISCV = \
	board.c \
	boot.c \
	copy.S \
	exception.S \
	interrupt.c \
	mcontext.c \
	pmap.c \
	sbi.c \
	sigcode.S \
	signal.c \
	start.S \
	switch.S \
	thread.c \
	tlb.c \
	trap.c

CLEAN-FILES += assym.h riscv.ld

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk

$(SOURCES-RISCV) riscv.ld.in: assym.h
