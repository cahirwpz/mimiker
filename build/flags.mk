# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile used to set the basic compilation flags.
#

ASFLAGS  += -Wall -Wextra -Werror
WFLAGS   += -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Werror \
	    -Wno-missing-field-initializers
CFLAGS   += -std=gnu11 -Og -ggdb3 -fomit-frame-pointer
CPPFLAGS += -DDEBUG
