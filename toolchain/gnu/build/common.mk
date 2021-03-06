VERSION = 1.4

CURL = curl -L --create-dirs
RM = rm -f -v
CP = cp -v

ifeq ($(shell uname),FreeBSD)
MAKEFLAGS = -j$(shell sysctl -n hw.ncpu)
else
MAKEFLAGS = -j$(shell nproc)
endif

ISL = isl-0.18
MPFR = mpfr-4.0.2
GMP = gmp-6.1.2
MPC = mpc-1.1.0
CLOOG = cloog-0.18.4
BINUTILS = binutils-2.32
GCC = gcc-9.2.0
GDB = gdb-10.1

SRCDIR := $(TOPDIR)/sources
HOSTDIR := $(TOPDIR)/host
PREFIX := /usr
