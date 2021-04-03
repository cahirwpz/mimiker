# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile used to establish values for variables that configure
# build system for given platform.
#

BOARD ?= malta

ifeq ($(BOARD), malta)
ARCH := mips
endif

ifeq ($(BOARD), rpi3)
ARCH := aarch64
endif

ifeq ($(BOARD), pc)
ARCH := amd64
endif

VERBOSE ?= 0
CLANG ?= 0
LOCKDEP ?= 0
KASAN ?= 0
KGPROF ?= 0
