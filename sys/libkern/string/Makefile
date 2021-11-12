# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../../..)

CPPFLAGS += -I$(TOPDIR)/lib/libc/string

SRCDIR = $(TOPDIR)/lib/libc/string

BCOPY_FORMAT = S
MEMCPY_FORMAT = S
STRLEN_FORMAT = S

ifeq ($(wildcard $(SRCDIR)/$(ARCH)/bcopy.S),)
	BCOPY_FORMAT = c
endif

ifeq ($(wildcard $(SRCDIR)/$(ARCH)/memcpy.S),)
	MEMCPY_FORMAT = c
endif

ifeq ($(wildcard $(SRCDIR)/$(ARCH)/strlen.S),)
	STRLEN_FORMAT = c
endif

SOURCES = bcopy.$(BCOPY_FORMAT) \
	  memcpy.$(MEMCPY_FORMAT) \
	  strlen.$(STRLEN_FORMAT) \
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

include $(TOPDIR)/build/build.kern.mk
