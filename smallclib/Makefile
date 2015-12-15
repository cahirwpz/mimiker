P	= mips-mti-elf-
CC	= $(P)gcc -mips32r2 -EL -nostdlib

INCLUDES = -Ismallclib/include
INCLUDES += -I../ # For uart_raw.h

# Explicitly picking files to compile.
SOURCES  = smallclib/ctype/ctype_.c
SOURCES += smallclib/ctype/isalnum.c
SOURCES += smallclib/ctype/isalpha.c
SOURCES += smallclib/ctype/isascii.c
SOURCES += smallclib/ctype/isblank.c
SOURCES += smallclib/ctype/iscntrl.c
SOURCES += smallclib/ctype/isdigit.c
SOURCES += smallclib/ctype/isgraph.c
SOURCES += smallclib/ctype/islower.c
SOURCES += smallclib/ctype/isprint.c
SOURCES += smallclib/ctype/ispunct.c
SOURCES += smallclib/ctype/isspace.c
SOURCES += smallclib/ctype/isupper.c
SOURCES += smallclib/errno/errno.c
SOURCES += smallclib/math/s_fpclassify.c
SOURCES += smallclib/math/s_frexp.c
SOURCES += smallclib/misc/fini.c
SOURCES += smallclib/misc/init.c
SOURCES += smallclib/misc/setjmp.S
SOURCES += smallclib/stdio/___low_printf.c
SOURCES += smallclib/stdio/___low_sprintf.c
SOURCES += smallclib/stdio/__format_parser.c
SOURCES += smallclib/stdio/fflush.c
SOURCES += smallclib/stdio/fputc.c
SOURCES += smallclib/stdio/fwrite.c
SOURCES += smallclib/stdio/iostreams.c
SOURCES += smallclib/stdio/ioutils.c
SOURCES += smallclib/stdio/low_seek.c
SOURCES += smallclib/stdio/low_write.c
SOURCES += smallclib/stdio/perror.c
SOURCES += smallclib/stdio/printf.c
SOURCES += smallclib/stdio/puts.c
SOURCES += smallclib/stdio/snprintf.c
SOURCES += smallclib/stdio/sprintf.c
SOURCES += smallclib/stdlib/abs.c
SOURCES += smallclib/stdlib/atoi.c
SOURCES += smallclib/stdlib/atol.c
SOURCES += smallclib/stdlib/atoll.c
SOURCES += smallclib/stdlib/bsearch.c
SOURCES += smallclib/stdlib/qsort.c
SOURCES += smallclib/stdlib/rand.c
SOURCES += smallclib/stdlib/rand_data.c
SOURCES += smallclib/stdlib/strtod.c
SOURCES += smallclib/stdlib/strtol.c
SOURCES += smallclib/stdlib/strtoll.c
SOURCES += smallclib/string/bzero.c
SOURCES += smallclib/string/memchr.c
SOURCES += smallclib/string/memcpy.c
SOURCES += smallclib/string/memmove.c
SOURCES += smallclib/string/memset.c
SOURCES += smallclib/string/strcmp.c
SOURCES += smallclib/string/strcpy.c
SOURCES += smallclib/string/strerror.c
SOURCES += smallclib/string/strlen.c
SOURCES += smallclib/string/strncmp.c
SOURCES += smallclib/string/strspn.c
SOURCES += smallclib/string/strstr.c

# SOURCES += smallclib/stdlib/wctomb.c
# NOTE: wctomb IS needed by __format_parser, but it requires the whole locale as a dependency, and even environemnt access. Instead, we'll provide a custom wctomb which fails to represent any wide character.

# Glue layer
SOURCES += glue/write.c
SOURCES += glue/lseek.c
SOURCES += glue/wctomb.c

# Compile .c files in-tree.
OBJECTS = $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SOURCES)))

# -Werror disabled for building a third-party external lib
CFLAGS =  -Wall -DPIC32MZ
# No optimisations, emit debugging info.
CFLAGS += -O0 -g
# Disables some optimized function implementations
CFLAGS += -D__PREFER_SIZE_OVER_SPEED__

all: smallclib.a

smallclib.a: $(OBJECTS)
	ar rvs $@ $(OBJECTS)

.SUFFIXES: .c .S
.c.o:
	$(CC) $(CFLAGS) $(CDEFS) $(INCLUDES) -c $< -o $@
.S.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f smallclib.a
	rm -f $(OBJECTS)
