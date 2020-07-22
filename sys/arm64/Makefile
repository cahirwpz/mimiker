# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = context.c \
	  copy.c \
	  exception.c \
	  interrupt.c \
	  pmap.c \
	  rootdev.c \
	  $(BOARD).c \
	  signal.c \
	  start.S \
	  thread.c \
	  ucontext.c

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk
