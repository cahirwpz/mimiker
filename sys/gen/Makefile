# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = \
	boot.c \
	pmap.c \
	syscall.c \
	thread.c

include $(TOPDIR)/build/build.kern.mk

CPPFLAGS += -D_MACHDEP

boot.o : CFLAGS_KASAN =
boot.o : CFLAGS_KCSAN =
boot.o : CFLAGS_KGPROF =
boot.o : CFLAGS_KFT =
