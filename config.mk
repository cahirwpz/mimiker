# vim: tabstop=8 shiftwidth=8 noexpandtab:
#
# Common makefile used to establish values for variables that configure
# build system for given platform.
#

CONFIG_OPTS := MIPS AARCH64 RISCV KASAN LOCKDEP KCSAN KGPROF KFTRACE

BOARD ?= rpi3

MIPS ?= 0
AARCH64 ?= 0
RISCV ?= 0

ifeq ($(BOARD), malta)
ARCH := mips
MIPS := 1
# Clang does not generate debug info recognized by gdb :(
LLVM := 0
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
LLVM ?= 1
LOCKDEP ?= 0
KASAN ?= 0
KCSAN ?= 0
# Kernel function instrumentation options: ftrace, gprof
KFI ?=
TRAP_USER_ACCESS ?= 0
