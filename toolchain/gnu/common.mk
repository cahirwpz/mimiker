include $(TOPDIR)/config.mk

CURL = curl -L --create-dirs
RM = rm -f -v
CP = cp -v

ifeq ($(shell uname),FreeBSD)
MAKEFLAGS = -j$(shell sysctl -n hw.ncpu)
else
MAKEFLAGS = -j$(shell nproc)
endif

SRCDIR := $(TOPDIR)/sources
HOSTDIR := $(TOPDIR)/host
HOSTBUILD := $(TOPDIR)/host-build
PREFIX := /usr
