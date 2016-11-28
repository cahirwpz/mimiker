all: prog.uelf.o

include ../Makefile.common

PROGRAMS_C = prog.c misbehave.c fd_test.c
SOURCES_ASM = syscalls.S start.S

CFLAGS   = -std=gnu11 -O0 -Wall -Werror -fno-builtin -ffreestanding -mhard-float
LDFLAGS  = -nostdlib -T mimiker.ld
LDLIBS   = -lc -lgcc

SOURCES_OBJS = $(SOURCES_ASM:%.S=%.o)
PROGRAMS_OBJS = $(PROGRAMS_C:%.c=%.o)
PROGRAMS_UELFS = $(PROGRAMS_C:%.c=%.uelf)
PROGRAMS_UELF_OS = $(PROGRAMS_C:%.c=%.uelf.o)

all: $(PROGRAMS_UELF_OS) $(PROGRAMS_UELFS)

# Compiling the program source
%.o: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) -c -o $@ $<

# Linking the program according to the provided script
%.uelf: %.o start.o mimiker.ld syscalls.o
	@echo "[LD] $(DIR)$< -> $(DIR)$@"
	$(CC) $(LDFLAGS) start.o -o $@ $< syscalls.o $(LDLIBS)

# Treating the liked program as binary data, placing it in .rodata
# section, and embedding into an object file
%.uelf.o: %.uelf Makefile
	@echo "[LD] $(DIR)$< -> $(DIR)$@"
	$(OBJCOPY) -I binary --rename-section .data=.rodata,contents,readonly,data -O elf32-little --alt-machine-code=8 $< $@

clean:
	rm -f $(PROGRAMS_UELF_OS) $(PROGRAMS_UELFS) $(PROGRAMS_OBJS) $(SOURCES_OBJS)
