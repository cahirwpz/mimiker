# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile used to establish values for variables that configure
# build system for given platform.
#

CONFIG_OPTS := KASAN LOCKDEP KGPROF MIPS AARCH64 RISCV KCSAN

BOARD ?= malta

MIPS ?= 0
AARCH64 ?= 0
RISCV ?= 0

ifeq ($(BOARD), malta)
ARCH := mips
MIPS := 1
endif

ifeq ($(BOARD), rpi3)
ARCH := aarch64
AARCH64 := 1
endif

ifeq ($(BOARD), litex-riscv)
ARCH := riscv
RISCV := 1
XLEN := 32
endif

ifeq ($(BOARD), sifive_u)
ARCH := riscv
RISCV := 1
XLEN := 64
endif

VERBOSE ?= 0
CLANG ?= 0
LOCKDEP ?= 0
KASAN ?= 0
KGPROF ?= 0
KCSAN ?= 0
TRAP_USER_ACCESS ?= 0
