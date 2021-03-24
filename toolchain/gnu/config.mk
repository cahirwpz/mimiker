# vim: tabstop=8 shiftwidth=8 noexpandtab list:

VERSION = 1.5

# Libraries required to build the toolchain.
ISL = isl-0.18
MPFR = mpfr-4.0.2
GMP = gmp-6.1.2
MPC = mpc-1.1.0
CLOOG = cloog-0.18.4

# Cross tools composing the toolchain.
BINUTILS = binutils-2.35
GCC = gcc-11-20210314
GDB = gdb-10.1

ISL-URL = "http://isl.gforge.inria.fr/$(ISL).tar.xz"
MPFR-URL = "ftp://ftp.gnu.org/gnu/mpfr/$(MPFR).tar.xz"
GMP-URL = "https://gmplib.org/download/gmp/$(GMP).tar.xz"
MPC-URL = "https://ftp.gnu.org/gnu/mpc/$(MPC).tar.gz"
CLOOG-URL = "http://www.bastoul.net/cloog/pages/download/$(CLOOG).tar.gz"
BINUTILS-URL = "https://ftp.gnu.org/gnu/binutils/$(BINUTILS).tar.xz"
#TODO: revert to the old URL when GCC 11 is officially released
#GCC-URL = "https://ftp.gnu.org/gnu/gcc/$(GCC)/$(GCC).tar.xz"
GCC-URL = "ftp://ftp.fu-berlin.de/unix/languages/gcc/snapshots/11-20210314/$(GCC).tar.xz"
GDB-URL = "https://ftp.gnu.org/gnu/gdb/$(GDB).tar.xz"

TARGETS = mipsel aarch64 amd64
