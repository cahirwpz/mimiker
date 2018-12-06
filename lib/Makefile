# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

SUBDIR = libmimiker newlib

all: $(SUBDIR) install

# Dependencies between user programs are currently expressed here.
newlib: | libmimiker-install

include $(TOPDIR)/build/subdir.mk
