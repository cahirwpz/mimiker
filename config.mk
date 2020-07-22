BOARD ?= malta

ifeq ($(BOARD), malta)
ARCH ?= mips
PLATFORM ?= malta
endif

ifeq ($(BOARD), rpi3)
ARCH := arm64
PLATFORM := rpi3
endif
