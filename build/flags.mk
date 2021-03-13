ASFLAGS  += -Wall -Wextra -Werror
WFLAGS   += -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Werror
OPTFLAGS ?= -Og
CFLAGS   += -std=gnu11 $(OPTFLAGS) -ggdb3 -fomit-frame-pointer
CPPFLAGS += -DDEBUG
