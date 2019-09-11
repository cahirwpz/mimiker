# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

CPPFLAGS += -Iinclude

# Explicitly picking files to compile.
SOURCES  = ctype/ctype_.c
SOURCES += ctype/tolower_.c
SOURCES += ctype/toupper_.c
SOURCES += stdio/printf.c
SOURCES += stdio/scanf.c
SOURCES += stdlib/strtol.c
SOURCES += stdlib/strtoul.c
SOURCES += stdlib/qsort.c
SOURCES += stdlib/rand.c
SOURCES += string/bcopy.S
SOURCES += string/bzero.c
SOURCES += string/memchr.c
SOURCES += string/memcmp.c
SOURCES += string/memcpy.S
SOURCES += string/memset.c
SOURCES += string/strchr.c
SOURCES += string/strcmp.c
SOURCES += string/strcspn.c
SOURCES += string/strlcat.c
SOURCES += string/strlcpy.c
SOURCES += string/strlen.c
SOURCES += string/strncmp.c
SOURCES += string/strnlen.c
SOURCES += string/strrchr.c
SOURCES += string/strsep.c
SOURCES += string/strspn.c

include $(TOPDIR)/build/build.kobj.mk

build-here: libstdc.ka
