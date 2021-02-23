TARGET = mipsel-mimiker-elf

GCC_EXTRA_CONF = --with-arch=mips32r2 \
				 --with-abi=32 \
				 --with-float=hard

ROOTDIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

include ../common.mk
