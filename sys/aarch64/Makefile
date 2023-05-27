# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES-AARCH64 = \
	board.c \
	boot.c \
	context.c \
	copy.S \
	evec.S \
	interrupt.c \
	pmap.c \
	sigcode.S \
	signal.c \
	start.S \
	switch.S \
	timer.c \
	tlb.c \
	trap.c

SOURCES-KFT = \
	kft.c

CFLAGS.boot.c = -mcmodel=large
CFLAGS.board.c = -mcmodel=large

CLEAN-FILES += assym.h

include $(TOPDIR)/build/build.kern.mk

$(SOURCES-AARCH64): assym.h

CPPFLAGS += -D_MACHDEP

boot.o : CFLAGS_KASAN =
boot.o : CFLAGS_KCSAN =
boot.o : CFLAGS_KGPROF =
boot.o : CFLAGS_KFT =
kft.o : CFLAGS_KFT =
