# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SUBDIR = ctype stdio stdlib string
FORMAT-RECURSE = no

SOURCES = copystr.c

include $(TOPDIR)/build/build.kern.mk

# Since copystr function cannot modify stack and caller-saved registers
# it must be a leaf function, hence it cannot call KASAN functions.
copystr.o : CFLAGS_KASAN =
