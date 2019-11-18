ifndef ARCH
  $(error ARCH variable not defined. Have you included config.mk?)
endif

TOOLCHAIN_FOUND = $(shell which $(TARGET)-gcc > /dev/null; echo $$?)
ifneq ($(TOOLCHAIN_FOUND), 0)
  $(error $(TARGET) toolchain not found. Please refer to README.md)
endif

ifeq ($(CLANG), 1)
CLANG_FOUND = $(shell which clang > /dev/null; echo $$?)
ifneq ($(CLANG_FOUND), 0)
  $(error Clang not found. Please refer to README.md)
endif
endif


TARGET_GCC = $(TARGET)-gcc $(GCC_ABIFLAGS) -g
TARGET_CLANG = clang $(CLANG_ABIFLAGS) -g

# Pass "CLANG=1" at command line to switch kernel compiler to Clang.
ifeq ($(CLANG), 1)
CC_AS = $(TARGET_CLANG)
# The genassym script produces C code with asm statements that have
# garbage instructions, which Clang checks using its built-in assembler
# and refuses to compile. This option disables this check.
ASSYM_CFLAGS += -no-integrated-as
else
CC_AS = $(TARGET_GCC)
endif

CC       = $(CC_AS)
AS       = $(CC_AS)
LD       = $(TARGET_GCC)
AR       = $(TARGET)-ar
NM       = $(TARGET)-nm
GDB      = $(TARGET)-gdb
RANLIB	 = $(TARGET)-ranlib
READELF  = $(TARGET)-readelf
OBJCOPY  = $(TARGET)-objcopy
OBJDUMP  = $(TARGET)-objdump
STRIP    = $(TARGET)-strip

CP      = cp
CPIO    = cpio --format=crc
CSCOPE  = cscope -b
CTAGS   = ctags
FORMAT  = clang-format-7 -style=file 
INSTALL = install -D
LN	= ln
RM      = rm -f
TAR	= tar
SED	= sed
CURL	= curl -J -L
GIT     = git
PATCH   = patch
GENASSYM = $(TOPDIR)/sys/script/genassym.sh
YACC	= byacc
