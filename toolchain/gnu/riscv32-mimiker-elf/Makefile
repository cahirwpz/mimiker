# vim: tabstop=8 shiftwidth=8 noexpandtab list:

TOPDIR = $(realpath ..)

ARCH = riscv32
TARGET = riscv32-mimiker-elf

GCC_EXTRA_CONF = --with-arch=rv32ima_zicsr_zifencei \
		 --with-abi=ilp32

include $(TOPDIR)/target.mk
