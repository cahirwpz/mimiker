# vim: tabstop=8 shiftwidth=8 noexpandtab:

include ../Makefile.common

# Flags for the library.  __PREFER_SIZE_OVER_SPEED__ disables some
# optimized function implementations. -Werror is not enabled since
# this is an external library.
LIB_CFLAGS = -D__PREFER_SIZE_OVER_SPEED__

LIBNAME = libstdc
LIB_BUILD_DIR = $(LIBNAME)_build

# Find the path to smallclib within toolchains resources
LIB_PATH = $(realpath $(join $(dir $(firstword $(MAKEFILE_LIST))), smallclib))

$(info Using MIPS smallclib sources located at: $(LIB_PATH))

CPPFLAGS  = -I$(LIB_PATH)/include
CPPFLAGS += -I../include

# Explicitly picking files to compile.
LIB_SOURCES  = ctype/ctype_.c
LIB_SOURCES += stdio/__format_parser_int.c
LIB_SOURCES += string/memchr.c
LIB_SOURCES += string/memset.c
LIB_SOURCES += string/bzero.c
LIB_SOURCES += string/memcpy.c
LIB_SOURCES += string/strlen.c
LIB_SOURCES += string/strcpy.c
LIB_SOURCES += string/strcmp.c
LIB_SOURCES += string/strspn.c
LIB_SOURCES += stdlib/qsort.c

# LIB_SOURCES += stdlib/wctomb.c
# NOTE: wctomb IS needed by __format_parser, but it requires the whole
# locale as a dependency, and even environemnt access. Instead, we'll
# provide a custom wctomb which fails to represent any wide character.

# Extra utilities
EXTRA_SOURCES += wctomb.c
EXTRA_SOURCES += kprintf.c
EXTRA_SOURCES += snprintf.c

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
define emit_lib_rule
CFILE := $(1)
OFILE := $(patsubst %.S,%.o,$(patsubst %.c,%.o,$(notdir $(1))))
$(LIB_BUILD_DIR)/$$(OFILE): $(LIB_PATH)/$$(CFILE) | $(LIB_BUILD_DIR)
	@echo "[CC] $$< -> $$@"
	$(CC) $(LIB_CFLAGS) $(CFLAGS) $(CPPFLAGS) -c $$^ -o $$@
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

clean:
	$(RM) $(LIBNAME).a
	$(RM) $(EXTRA_OBJECTS)
	$(RM) $(LIB_OBJECTS)
	$(RM) *~ tags *.taghl
	-rmdir $(LIB_BUILD_DIR)
