# vim: tabstop=8 shiftwidth=8 noexpandtab list:

TOPDIR = $(realpath ..)

ARCH = riscv64
TARGET = riscv64-mimiker-elf

GCC_EXTRA_CONF = --with-arch=rv64ima \
		 --with-abi=lp64 \
		 --with-cmodel=medany

include $(TOPDIR)/target.mk
