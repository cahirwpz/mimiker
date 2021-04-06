# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile used to establish the implementation of the basic
# tools used throughout the build system.
#
# The following make variables are set by the including makefile:
# - TARGET, {CLANG,GCC}_ABIFLAGS: Set by arch.*.mk files.

ifndef ARCH
  $(error ARCH variable not defined. Have you included config.mk?)
endif

GCC_FOUND = $(shell which $(TARGET)-gcc > /dev/null; echo $$?)
ifneq ($(GCC_FOUND), 0)
  $(error $(TARGET)-gcc compiler not found - please refer to README.md!)
endif

# Pass "CLANG=1" at command line to switch kernel compiler to Clang.
ifeq ($(CLANG), 1)
CLANG_FOUND = $(shell which clang > /dev/null; echo $$?)
ifneq ($(CLANG_FOUND), 0)
  $(error clang compiler not found - please refer to README.md!)
endif
TARGET_CC = clang $(CLANG_ABIFLAGS) -g
# The genassym script produces C code with asm statements that have
# garbage instructions, which Clang checks using its built-in assembler
# and refuses to compile. This option disables this check.
ASSYM_CFLAGS += -no-integrated-as
else
TARGET_CC = $(TARGET)-gcc $(GCC_ABIFLAGS) -g
endif

CC       = $(TARGET_CC)
AS       = $(TARGET_CC)
LD       = $(TARGET)-gcc $(GCC_ABIFLAGS) -g
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
FORMAT  = clang-format -style=file 
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
GZIP	= gzip -9
