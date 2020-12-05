# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = boot.c \
	  context.c \
	  copy.S \
	  emmc.c \
	  evec.S \
	  interrupt.c \
	  gpio.c \
	  pl011.c \
	  pmap.c \
	  rootdev.c \
	  $(BOARD).c \
	  signal.c \
	  start.S \
	  switch.S \
	  timer.c \
	  thread.c \
	  tlb.c \
	  trap.c \
	  ucontext.c

CFLAGS.boot.c = -mcmodel=large
CFLAGS.rpi3.c = -mcmodel=large

CLEAN-FILES += assym.h

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk

$(SOURCES): assym.h
