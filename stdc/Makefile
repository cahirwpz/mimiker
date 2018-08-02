# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

include $(TOPDIR)/build/build.kern.mk

# Flags for the library.  __PREFER_SIZE_OVER_SPEED__ disables some
# optimized function implementations. -Werror is not enabled since
# this is an external library.
LIB_CFLAGS = -D__PREFER_SIZE_OVER_SPEED__

LIBNAME = libstdc

CPPFLAGS += -Iinclude

# Explicitly picking files to compile.
LIB_SOURCES  = ctype/ctype_.c
LIB_SOURCES += ctype/tolower_.c
LIB_SOURCES += ctype/toupper_.c
LIB_SOURCES += stdio/__format_parser_int.c
LIB_SOURCES += stdio/__scanf_core_int.c
LIB_SOURCES += stdlib/strtol.c
LIB_SOURCES += stdlib/strtoul.c
LIB_SOURCES += stdlib/qsort.c
LIB_SOURCES += stdlib/rand.c
LIB_SOURCES += string/bzero.c
LIB_SOURCES += string/memchr.c
LIB_SOURCES += string/memcpy.c
LIB_SOURCES += string/memset.c
LIB_SOURCES += string/strchr.c
LIB_SOURCES += string/strcmp.c
LIB_SOURCES += string/strcspn.c
LIB_SOURCES += string/strlcat.c
LIB_SOURCES += string/strlcpy.c
LIB_SOURCES += string/strlen.c
LIB_SOURCES += string/strncmp.c
LIB_SOURCES += string/strnlen.c
LIB_SOURCES += string/strrchr.c
LIB_SOURCES += string/strsep.c
LIB_SOURCES += string/strspn.c
LIB_SOURCES += string/strstr.c

# LIB_SOURCES += stdlib/wctomb.c
# NOTE: wctomb IS needed by __format_parser, but it requires the whole
# locale as a dependency, and even environemnt access. Instead, we'll
# provide a custom wctomb which fails to represent any wide character.

# Extra utilities
EXTRA_SOURCES += wctomb.c
EXTRA_SOURCES += kprintf.c
EXTRA_SOURCES += snprintf.c
EXTRA_SOURCES += sscanf.c


# The default rule.
all: $(LIBNAME).a

# This rule creates the library archive. To define it, we need a full
# list of all object files.
LIB_OBJECTS = $(patsubst %.S,%.o, $(patsubst %.c,%.o, $(LIB_SOURCES)))
EXTRA_OBJECTS = $(patsubst %.S,%.o, $(patsubst %.c,%.o, $(EXTRA_SOURCES)))

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
OFILE := $(patsubst %.S,%.o, $(patsubst %.c,%.o, $(1)))
$$(OFILE): $$(CFILE)
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
