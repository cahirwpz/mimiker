TARGET = mipsel-mimiker-elf

TOOLCHAIN_FOUND = $(shell which $(TARGET)-gcc > /dev/null; echo $$?)
ifneq ($(TOOLCHAIN_FOUND), 0)
  $(error $(TARGET) toolchain not found. Please refer to README.md)
endif

CC       = $(TARGET)-gcc -mips32r2 -EL -g
AR       = $(TARGET)-ar
AS       = $(TARGET)-as -mips32r2 -EL -g
NM       = $(TARGET)-nm
GDB      = $(TARGET)-gdb
RANLIB	 = $(TARGET)-ranlib
READELF  = $(TARGET)-readelf
OBJCOPY  = $(TARGET)-objcopy
OBJDUMP  = $(TARGET)-objdump
