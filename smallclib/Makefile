P	= mips-mti-elf-
CC	= $(P)gcc -mips32r2 -EL -nostdlib
AR  = $(P)ar

# -Werror disabled for building a third-party external lib
CFLAGS =  -Wall -DPIC32MZ
# No optimisations, emit debugging info.
CFLAGS += -O0 -g
# Disables some optimized function implementations
CFLAGS += -D__PREFER_SIZE_OVER_SPEED__

# Find the path to smallclib within toolchains resources
HEADER_PATH = $(dir $(shell mips-mti-elf-gcc --print-libgcc-file-name))
SMALLCLIB_PATH = $(realpath $(join $(HEADER_PATH), ../../../../src/smallclib/))

$(info "Using smallclib sources located at: " $(SMALLCLIB_PATH))

SMALLCLIB_BUILD_DIR = smallclib_build

INCLUDES = -I$(SMALLCLIB_PATH)/include
INCLUDES += -I../ # For uart_raw.h

# Explicitly picking files to compile.
LIB_SOURCES  = ctype/ctype_.c
LIB_SOURCES += ctype/isalnum.c
LIB_SOURCES += ctype/isalpha.c
LIB_SOURCES += ctype/isascii.c
LIB_SOURCES += ctype/isblank.c
LIB_SOURCES += ctype/iscntrl.c
LIB_SOURCES += ctype/isdigit.c
LIB_SOURCES += ctype/isgraph.c
LIB_SOURCES += ctype/islower.c
LIB_SOURCES += ctype/isprint.c
LIB_SOURCES += ctype/ispunct.c
LIB_SOURCES += ctype/isspace.c
LIB_SOURCES += ctype/isupper.c
LIB_SOURCES += errno/errno.c
LIB_SOURCES += math/s_fpclassify.c
LIB_SOURCES += math/s_frexp.c
LIB_SOURCES += misc/fini.c
LIB_SOURCES += misc/init.c
LIB_SOURCES += misc/setjmp.S
LIB_SOURCES += stdio/___low_printf.c
LIB_SOURCES += stdio/___low_sprintf.c
LIB_SOURCES += stdio/__format_parser.c
LIB_SOURCES += stdio/fflush.c
LIB_SOURCES += stdio/fputc.c
LIB_SOURCES += stdio/fwrite.c
LIB_SOURCES += stdio/iostreams.c
LIB_SOURCES += stdio/ioutils.c
LIB_SOURCES += stdio/low_seek.c
LIB_SOURCES += stdio/low_write.c
LIB_SOURCES += stdio/perror.c
LIB_SOURCES += stdio/printf.c
LIB_SOURCES += stdio/puts.c
LIB_SOURCES += stdio/snprintf.c
LIB_SOURCES += stdio/sprintf.c
LIB_SOURCES += stdlib/abs.c
LIB_SOURCES += stdlib/atoi.c
LIB_SOURCES += stdlib/atol.c
LIB_SOURCES += stdlib/atoll.c
LIB_SOURCES += stdlib/bsearch.c
LIB_SOURCES += stdlib/qsort.c
LIB_SOURCES += stdlib/rand.c
LIB_SOURCES += stdlib/rand_data.c
LIB_SOURCES += stdlib/strtod.c
LIB_SOURCES += stdlib/strtol.c
LIB_SOURCES += stdlib/strtoll.c
LIB_SOURCES += string/bzero.c
LIB_SOURCES += string/memchr.c
LIB_SOURCES += string/memcpy.c
LIB_SOURCES += string/memmove.c
LIB_SOURCES += string/memset.c
LIB_SOURCES += string/strcmp.c
LIB_SOURCES += string/strcpy.c
LIB_SOURCES += string/strerror.c
LIB_SOURCES += string/strlen.c
LIB_SOURCES += string/strncmp.c
LIB_SOURCES += string/strspn.c
LIB_SOURCES += string/strstr.c

LIB_SOURCES_FULL = $(addprefix $(SMALLCLIB_PATH)/, $(LIB_SOURCES))

# LIB_SOURCES += smallclib/stdlib/wctomb.c
# NOTE: wctomb IS needed by __format_parser, but it requires the whole
# locale as a dependency, and even environemnt access. Instead, we'll
# provide a custom wctomb which fails to represent any wide character.

# Glue layer
GLUE_SOURCES += glue/write.c
GLUE_SOURCES += glue/lseek.c
GLUE_SOURCES += glue/wctomb.c

# The default rule.
all: smallclib.a

# This rule creates the build directory.
$(SMALLCLIB_BUILD_DIR):
	mkdir -p $(SMALLCLIB_BUILD_DIR)

# This rule creates the library archive. To define it, we need a full
# list of all object files.
LIB_OBJECTS  = \
	$(addprefix $(SMALLCLIB_BUILD_DIR)/,\
		$(patsubst %.S,%.o,             \
		$(patsubst %.c,%.o,             \
			$(notdir $(LIB_SOURCES))    \
		) \
		) \
	)
GLUE_OBJECTS = \
	$(patsubst %.S,%.o, \
	$(patsubst %.c,%.o, \
		$(GLUE_SOURCES) \
	) \
	)
smallclib.a: $(GLUE_OBJECTS) $(LIB_OBJECTS)
	@echo "Creating $@"
	$(AR) rvs $@ $(GLUE_OBJECTS) $(LIB_OBJECTS) > /dev/null

# Generate rules for each library file separately. A simple pattern
# rule will not suffice, because we need to correct the file directory
# for rule and dependency.
define emit_smallclib_rule =
CFILE := $(1)
OFILE := $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(notdir $(1))))
$(SMALLCLIB_BUILD_DIR)/$(OFILE): $(SMALLCLIB_PATH)/$(CFILE) $(SMALLCLIB_BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEFS) $(INCLUDES) -c $(SMALLCLIB_PATH)/$(CFILE) -o $(SMALLCLIB_BUILD_DIR)/$(OFILE)
endef

# Now loop over LIB_SOURCES, and create a rule for each.  For some
# reason foreach ignores the last element of the list (probably
# variables are evaluated after the rule is emitted), so the list is
# extended with a 'null' element to fix this.  For more information
# about function evaluation and rule generation, please see
# http://www.gnu.org/software/make/manual/make.html#Eval-Function
$(foreach file, $(LIB_SOURCES) null, $(eval $(call emit_smallclib_rule,$(file))))

# Here is a pattern rule for the glue layer. The glue layer is build
# in-tree, so a pattern is enough.
glue/%.o: glue/%.c
	$(CC) $(CFLAGS) $(CDEFS) $(INCLUDES) -c $< -o $@

clean:
	rm -f smallclib.a
	rm -f $(LIB_OBJECTS)
	rm -f $(GLUE_OBJECTS)
	rmdir $(SMALLCLIB_BUILD_DIR)
