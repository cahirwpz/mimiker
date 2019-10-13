ASFLAGS  +=
WFLAGS   += -Wall -Wextra -Wno-unused-parameter -Wstrict-prototypes -Werror
CFLAGS   += -std=gnu11 -Og -ggdb3 $(WFLAGS) -fno-asynchronous-unwind-tables
CPPFLAGS += -Wall -Werror -DDEBUG
