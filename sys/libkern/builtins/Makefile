# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../../..)

SRCDIR = $(TOPDIR)/lib/libc/builtins

SOURCES = bswapsi2.c clzsi2.c ctzsi2.c clzdi2.c ffssi2.c udivdi3.c umoddi3.c

include $(TOPDIR)/build/build.kern.mk

CPPFLAGS += -I$(CURDIR)/../builtins
