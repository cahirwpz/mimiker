# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

CPPFLAGS += -Iinclude

SOURCES = \
	fdt.c \
	fdt_addresses.c \
	fdt_ro.c \
	fdt_strerror.c

FORMAT-EXCLUDE = $(SOURCES)

include $(TOPDIR)/build/build.kern.mk
