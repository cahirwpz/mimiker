# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

# Flags for the library.  __PREFER_SIZE_OVER_SPEED__ disables some
# optimized function implementations. -Werror is not enabled since
# this is an external library.
CFLAGS += -D__PREFER_SIZE_OVER_SPEED__
CPPFLAGS += -Iinclude

# Explicitly picking files to compile.
SOURCES  = ctype/ctype_.c
SOURCES += ctype/tolower_.c
SOURCES += ctype/toupper_.c
SOURCES += stdio/__format_parser_int.c
SOURCES += stdio/__scanf_core_int.c
SOURCES += stdlib/strtol.c
SOURCES += stdlib/strtoul.c
SOURCES += stdlib/qsort.c
SOURCES += stdlib/rand.c
SOURCES += string/bzero.c
SOURCES += string/memchr.c
SOURCES += string/memcmp.c
SOURCES += string/memcpy.c
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

# SOURCES += stdlib/wctomb.c
# NOTE: wctomb IS needed by __format_parser, but it requires the whole
# locale as a dependency, and even environemnt access. Instead, we'll
# provide a custom wctomb which fails to represent any wide character.

SOURCES += wctomb.c
SOURCES += kprintf.c
SOURCES += snprintf.c
SOURCES += sscanf.c

include $(TOPDIR)/build/build.kobj.mk

build-here: libstdc.ka
