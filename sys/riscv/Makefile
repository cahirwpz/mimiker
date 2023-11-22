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
	tlb.c \
	trap.c

CLEAN-FILES += assym.h riscv.ld

include $(TOPDIR)/build/build.kern.mk

$(SOURCES-RISCV) : assym.h

CPPFLAGS += -D_MACHDEP

boot.o : CFLAGS_KASAN =
boot.o : CFLAGS_KCSAN =
boot.o : CFLAGS_KFI =
