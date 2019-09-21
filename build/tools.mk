ifndef ARCH
  $(error ARCH variable not defined. Have you included config.mk?)
endif

TOOLCHAIN_FOUND = $(shell which $(TARGET)-gcc > /dev/null; echo $$?)
ifneq ($(TOOLCHAIN_FOUND), 0)
  $(error $(TARGET) toolchain not found. Please refer to README.md)
endif

CC       = $(TARGET)-gcc $(ABIFLAGS) -g
AR       = $(TARGET)-ar
AS       = $(TARGET)-gcc $(ABIFLAGS) -g
LD       = $(TARGET)-gcc $(ABIFLAGS) -g
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
