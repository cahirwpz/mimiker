# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SUBDIR = ctype stdio stdlib string
FORMAT-RECURSE = no

SOURCES = copystr.c

include $(TOPDIR)/build/build.kern.mk

# This function must be compiled without KASAN because it is used by copyinstr
# which cannot failed.
copystr.o : CFLAGS_KASAN =
