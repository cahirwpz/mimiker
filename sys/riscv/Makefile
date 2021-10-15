# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES-RISCV = \
	boot.c \
	copy.S \
	evec.S \
	interrupt.c \
	mcontext.c \
	pmap.c \
	signal.c \
	start.S \
	thread.c

CLEAN-FILES += assym.h

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk

$(SOURCES-RISCV) riscv.ld: assym.h
