# vim: tabstop=8 shiftwidth=8 noexpandtab list:

VERSION = 1.5.3

# Libraries required to build the toolchain.
ISL = isl-0.18
MPFR = mpfr-4.0.2
GMP = gmp-6.1.2
MPC = mpc-1.1.0
CLOOG = cloog-0.18.4

# The toolchain is comprised of following packages:
BINUTILS = binutils-2.37
GCC = gcc-11.2.0
GDB = gdb-11.1

ISL-URL = "https://gcc.gnu.org/pub/gcc/infrastructure/$(ISL).tar.bz2"
MPFR-URL = "ftp://ftp.gnu.org/gnu/mpfr/$(MPFR).tar.xz"
GMP-URL = "https://gmplib.org/download/gmp/$(GMP).tar.xz"
MPC-URL = "https://ftp.gnu.org/gnu/mpc/$(MPC).tar.gz"
CLOOG-URL = "http://www.bastoul.net/cloog/pages/download/$(CLOOG).tar.gz"
BINUTILS-URL = "https://ftp.gnu.org/gnu/binutils/$(BINUTILS).tar.xz"
GCC-URL = "https://ftp.gnu.org/gnu/gcc/$(GCC)/$(GCC).tar.xz"
GDB-URL = "https://ftp.gnu.org/gnu/gdb/$(GDB).tar.xz"

TARGETS = mipsel aarch64 riscv32
