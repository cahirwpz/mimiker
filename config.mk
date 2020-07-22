BOARD ?= malta

ifeq ($(BOARD), malta)
ARCH := mips
endif

ifeq ($(BOARD), rpi3)
ARCH := arm64
endif
