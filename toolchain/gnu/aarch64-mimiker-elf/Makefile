TARGET = aarch64-mimiker-elf

GCC_EXTRA_CONF = --with-arch=armv8-a \
		 --with-cpu=cortex-a53

ROOTDIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

include ../common.mk
