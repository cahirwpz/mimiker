# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = \
	boot.c \
	pmap.c \
	thread.c

CPPFLAGS += -D_MACHDEP

include $(TOPDIR)/build/build.kern.mk

boot.o : CFLAGS_KASAN =
boot.o : CFLAGS_KCSAN =
boot.o : CFLAGS_KGPROF =
