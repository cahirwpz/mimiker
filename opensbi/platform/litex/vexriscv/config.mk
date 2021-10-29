# vim: tabstop=8 shiftwidth=8 noexpandtab:

# Compiler flags.
platform-cppflags-y =
platform-cflags-y =
platform-asflags-y =
platform-ldflags-y =

# 
# Platform RISC-V XLEN, ABI, ISA and Code Model configuration
# are obtained based on GCC compiler capabilities.
#

# Firmware load address configuration.
FW_TEXT_START=0x80000000

# We use the jump firmware configuration.
FW_JUMP=y

# This needs to be 4MB aligned for 32-bit system.
FW_JUMP_ADDR=$(shell printf "0x%X" $$(($(FW_TEXT_START) + 0x400000)))

# Use the same address as QEMU.
FW_JUMP_FDT_ADDR=$(shell printf "0x%X" $$(($(FW_TEXT_START) + 0x2200000)))
