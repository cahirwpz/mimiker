# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

SUBDIR = libmimiker newlib

all: build

# Dependencies between user programs are currently expressed here.
newlib-before: libmimiker-install

include $(TOPDIR)/build/common.mk
