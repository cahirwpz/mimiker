BOARD ?= malta

ifeq ($(BOARD), malta)
ARCH := mips
endif

ifeq ($(BOARD), rpi3)
ARCH := aarch64
endif
