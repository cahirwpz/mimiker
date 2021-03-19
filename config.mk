# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile used to establish the target architecture
# and set the default value of the lock dependency validator flag.
#
# The following make variables are assumed to be set:
# -BOARD: The target board.

BOARD ?= malta

ifeq ($(BOARD), malta)
ARCH := mips
endif

ifeq ($(BOARD), rpi3)
ARCH := aarch64
endif

LOCKDEP ?= 0
