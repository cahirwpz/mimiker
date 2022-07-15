# vim: tabstop=2 shiftwidth=2 noexpandtab:
#
# This is a common makefile used to establish the implementation of the basic
# tools used throughout the build system.
#
# The following make variables are set by the including makefile:
# - TARGET, ABIFLAGS: Set by arch.*.mk files.

LLVM_VER := -14

ifndef ARCH
  $(error ARCH variable not defined. Have you included config.mk?)
endif

# Pass "LLVM=1" at command line to switch to llvm toolchain.
ifeq ($(LLVM), 1)

ifneq ($(shell which clang$(LLVM_VER) > /dev/null; echo $$?), 0)
  $(error clang compiler not found - please refer to README.md!)
endif

ifneq ($(shell which ld.lld$(LLVM_VER) > /dev/null; echo $$?), 0)
  $(error lld linker not found)
endif

ifneq ($(shell which llvm-ar$(LLVM_VER) > /dev/null; echo $$?), 0)
  $(error llvm toolchain not found)
endif

CC	= clang$(LLVM_VER) -target $(TARGET) $(ABIFLAGS) -g
CPP	= $(CC) -x c -E
AS	= $(CC)
LD	= ld.lld$(LLVM_VER)
AR	= llvm-ar$(LLVM_VER)
NM	= llvm-nm$(LLVM_VER)
RANLIB	= llvm-ranlib$(LLVM_VER)
READELF	= llvm-readelf$(LLVM_VER)
OBJCOPY	= llvm-objcopy$(LLVM_VER)
OBJDUMP	= llvm-objdump$(LLVM_VER)
STRIP	= llvm-strip$(LLVM_VER)

# The genassym script produces C code with asm statements that have
# garbage instructions, which Clang checks using its built-in assembler
# and refuses to compile. This option disables this check.
ASSYM_CFLAGS += -no-integrated-as

else

ifneq ($(shell which $(TARGET)-gcc > /dev/null; echo $$?), 0)
  $(error $(TARGET)-gcc compiler not found - please refer to README.md!)
endif

CC	= $(TARGET)-gcc $(ABIFLAGS) -g
CPP	= $(TARGET)-cpp
AS	= $(CC)
LD	= $(TARGET)-ld
AR	= $(TARGET)-ar
NM	= $(TARGET)-nm
RANLIB	= $(TARGET)-ranlib
READELF	= $(TARGET)-readelf
OBJCOPY	= $(TARGET)-objcopy
OBJDUMP	= $(TARGET)-objdump
STRIP	= $(TARGET)-strip
endif

CP      = cp
CPIO    = cpio --format=crc
CSCOPE  = cscope -b
CTAGS   = ctags
FORMAT  = clang-format$(LLVM_VER) -style=file 
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
