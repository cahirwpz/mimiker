P	= mips-mti-elf-
CC	= $(P)gcc -mips32r2 -EL -nostdlib
AR  = $(P)ar

# -Werror disabled for building a third-party external lib
CFLAGS =  -Wall -DPIC32MZ
# No optimisations, emit debugging info.
CFLAGS += -O0 -g
# Disables some optimized function implementations
CFLAGS += -D__PREFER_SIZE_OVER_SPEED__

LIBNAME = smallclib
LIB_BUILD_DIR = $(LIBNAME)_build

# Find the path to smallclib within toolchains resources
STD_HEADER_PATH = $(dir $(shell mips-mti-elf-gcc --print-libgcc-file-name))
LIB_PATH = $(realpath $(join $(STD_HEADER_PATH), ../../../../src/smallclib/))

$(info Using $(LIBNAME) sources located at: $(LIB_PATH))

INCLUDES = -I$(LIB_PATH)/include
INCLUDES += -isystemextra
INCLUDES += -I../ # For uart_raw.h

# Explicitly picking files to compile.
LIB_SOURCES  = ctype/ctype_.c
LIB_SOURCES += math/s_fpclassify.c
LIB_SOURCES += math/s_frexp.c
LIB_SOURCES += stdio/___low_sprintf.c
LIB_SOURCES += stdio/__format_parser.c
LIB_SOURCES += stdio/__format_parser_int.c
LIB_SOURCES += stdio/sprintf.c
LIB_SOURCES += string/memchr.c
LIB_SOURCES += string/memset.c
LIB_SOURCES += string/strlen.c
LIB_SOURCES += string/strspn.c

# LIB_SOURCES += stdlib/wctomb.c
# NOTE: wctomb IS needed by __format_parser, but it requires the whole
# locale as a dependency, and even environemnt access. Instead, we'll
# provide a custom wctomb which fails to represent any wide character.

# Extra utilities
EXTRA_SOURCES += extra/wctomb.c
EXTRA_SOURCES += extra/kperror.c
EXTRA_SOURCES += extra/kputs.c
EXTRA_SOURCES += extra/kprintf.c

# The default rule.
all: $(LIBNAME).a

# This rule creates the build directory.
$(LIB_BUILD_DIR):
	mkdir -p $(LIB_BUILD_DIR)

# This rule creates the library archive. To define it, we need a full
# list of all object files.
LIB_OBJECTS  = \
	$(addprefix $(LIB_BUILD_DIR)/,\
		$(patsubst %.S,%.o,             \
		$(patsubst %.c,%.o,             \
			$(notdir $(LIB_SOURCES))    \
		) \
		) \
	)
EXTRA_OBJECTS = \
	$(patsubst %.S,%.o, \
	$(patsubst %.c,%.o, \
		$(EXTRA_SOURCES) \
	) \
	)
$(LIBNAME).a: $(EXTRA_OBJECTS) $(LIB_OBJECTS)
	@echo "Creating $@"
	$(AR) rvs $@ $(EXTRA_OBJECTS) $(LIB_OBJECTS) > /dev/null

# Generate rules for each library file separately. A simple pattern
# rule will not suffice, because we need to correct the file directory
# for rule and dependency.  The | char turns the directory dependency
# into an order-only dependency. This way the directory is required to
# be created before compiling the file, but a newer timestamp on the
# directory will not cause the file to be considered out of date. If
# the dependency on the directory was standard one, each file would be
# recompiled whenever the timestamp on the dir is bumped, which
# happens every time a new file lands within the dir.  For more
# information or order-only dependencies, please see
# http://www.gnu.org/software/make/manual/make.html#Prerequisite-Types
define emit_lib_rule =
CFILE := $(1)
OFILE := $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(notdir $(1))))
$(LIB_BUILD_DIR)/$(OFILE): $(LIB_PATH)/$(CFILE) | $(LIB_BUILD_DIR)
	$(CC) $(CFLAGS) $(CDEFS) $(INCLUDES) -c $(LIB_PATH)/$(CFILE) -o $(LIB_BUILD_DIR)/$(OFILE)
endef

# Now loop over LIB_SOURCES, and create a rule for each.  For some
# reason foreach ignores the last element of the list (probably
# variables are evaluated after the rule is emitted), so the list is
# extended with a 'null' element to fix this.  For more information
# about function evaluation and rule generation, please see
# http://www.gnu.org/software/make/manual/make.html#Eval-Function The
# documentation for GNU make's foreach function is available at
# https://www.gnu.org/software/make/manual/html_node/Foreach-Function.html
$(foreach file, $(LIB_SOURCES) null, $(eval $(call emit_lib_rule,$(file))))

# Here is a pattern rule for the glue layer. The glue layer is build
# in-tree, so a pattern is enough.
extra/%.o: extra/%.c
	$(CC) $(CFLAGS) $(CDEFS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(LIBNAME).a
	rm -f $(LIB_OBJECTS)
	rm -f $(GLUE_OBJECTS)
	rmdir $(LIB_BUILD_DIR)
