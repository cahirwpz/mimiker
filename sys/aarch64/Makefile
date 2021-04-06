# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = boot.c \
	  context.c \
	  copy.S \
	  evec.S \
	  interrupt.c \
	  gpio.c \
	  pl011.c \
	  pmap.c \
	  rootdev.c \
	  rpi3.c \
	  sigcode.S \
	  signal.c \
	  start.S \
	  switch.S \
	  timer.c \
	  thread.c \
	  tlb.c \
	  trap.c

CFLAGS.boot.c = -mcmodel=large
CFLAGS.rpi3.c = -mcmodel=large

CLEAN-FILES += assym.h

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk

ifeq ($(ARCH), aarch64)
$(SOURCES): assym.h
endif

boot.o : CFLAGS_KASAN =
