# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

DTS = $(BOARD).dts
DTB = $(BOARD).dtb

ifeq ($(wildcard $(DTS)),)
	DTB = 
endif

BUILD-FILES += $(DTB)

CLEAN-FILES += $(DTB)

include $(TOPDIR)/build/build.kern.mk
