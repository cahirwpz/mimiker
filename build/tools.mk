# vim: tabstop=2 shiftwidth=2 noexpandtab:
#
# This is a common makefile used to establish the implementation of the basic
# tools used throughout the build system.
#
# The following make variables are set by the including makefile:
# - TARGET, {CLANG,GCC}_ABIFLAGS: Set by arch.*.mk files.

ifndef ARCH
  $(error ARCH variable not defined. Have you included config.mk?)
endif

# Pass "LLVM=1" at command line to switch to llvm toolchain.
ifeq ($(LLVM), 1)

ifneq ($(shell which clang > /dev/null; echo $$?), 0)
  $(error clang compiler not found - please refer to README.md!)
endif

ifneq ($(shell which lld > /dev/null; echo $$?), 0)
  $(error lld linker not found)
endif

ifneq ($(shell which llvm-ar > /dev/null; echo $$?), 0)
  $(error llvm toolchain not found)
endif

CC	= clang $(CLANG_ABIFLAGS) -g
CPP	= $(CC) -E
AS	= $(CC)
LD	= $(CC) -B /usr/bin -fuse-ld=lld
AR	= llvm-ar
NM	= llvm-nm
RANLIB	= llvm-ranlib
READELF	= llvm-readelf
OBJCOPY	= llvm-objcopy
OBJDUMP	= llvm-objdump
STRIP	= llvm-strip

# The genassym script produces C code with asm statements that have
# garbage instructions, which Clang checks using its built-in assembler
# and refuses to compile. This option disables this check.
ASSYM_CFLAGS += -no-integrated-as

else

ifneq ($(shell which $(TARGET)-gcc > /dev/null; echo $$?), 0)
  $(error $(TARGET)-gcc compiler not found - please refer to README.md!)
endif

CC	= $(TARGET)-gcc $(GCC_ABIFLAGS) -g
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
