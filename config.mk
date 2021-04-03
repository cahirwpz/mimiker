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

LOCKDEP ?= 0
