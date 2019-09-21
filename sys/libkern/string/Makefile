# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../../..)

CPPFLAGS += -I../include

SOURCES = $(ARCH)/bcopy.S \
	  $(ARCH)/memcpy.S \
	  bzero.c \
	  memchr.c \
	  memcmp.c \
	  memset.c \
	  strchr.c \
	  strcmp.c \
	  strcspn.c \
	  strlcat.c \
	  strlcpy.c \
	  strlen.c \
	  strncmp.c \
	  strnlen.c \
	  strrchr.c \
	  strsep.c \
	  strspn.c

include $(TOPDIR)/build/build.kern.mk
