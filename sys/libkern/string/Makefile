# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../../..)

CPPFLAGS += -I$(TOPDIR)/lib/libc/string

SRCDIR = $(TOPDIR)/lib/libc/string

SOURCES = \
	bzero.c \
	memchr.c \
	memcmp.c \
	memset.c \
	strchr.c \
	strcmp.c \
	strcspn.c \
	strlcat.c \
	strlcpy.c \
	strncmp.c \
	strncpy.c \
	strnlen.c \
	strrchr.c \
	strsep.c \
	strspn.c

SOURCES-AARCH64 = \
	bcopy.S \
	memcpy.S \
	strlen.S

SOURCES-MIPS = \
	bcopy.S \
	memcpy.S \
	strlen.S

SOURCES-RISCV = \
	bcopy.c \
	memcpy.c \
	strlen.c

include $(TOPDIR)/build/build.kern.mk
