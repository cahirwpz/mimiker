# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# This is a common makefile used to establish the target architecture
# and set the default value of the lock dependency validator flag.
#
# The following make variables are set by the including makefile:
# -BOARD: The target board. Defaults to malta.
# -LOCKDEP: The lock dependency validator flag. Defaults to 0.

BOARD ?= malta

ifeq ($(BOARD), malta)
ARCH := mips
endif

ifeq ($(BOARD), rpi3)
ARCH := aarch64
endif

LOCKDEP ?= 0
KGPROF ?= 0
