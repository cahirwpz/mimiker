# vim: tabstop=8 shiftwidth=8 noexpandtab list:

TOPDIR = $(CURDIR)

all: package

include $(TOPDIR)/common.mk

build-host: gmp-install mpfr-install mpc-install isl-install cloog-install

clean-host: gmp-clean mpfr-clean mpc-clean isl-clean cloog-clean
	$(RM) -r $(HOSTDIR) $(HOSTBUILD)

build-%-mimiker-elf: build-host binutils-unpack gcc-unpack gdb-unpack
	$(MAKE) -C $*-mimiker-elf

%-mimiker-elf_$(VERSION)_amd64.deb: build-%-mimiker-elf
	$(MAKE) -C $*-mimiker-elf package

clean-%-mimiker-elf:
	$(MAKE) -C $*-mimiker-elf clean
	$(RM) $*-.deb

build: $(TARGETS:%=build-%-mimiker-elf)

package: $(TARGETS:%=%-mimiker-elf_$(VERSION)_amd64.deb)

clean: $(TARGETS:%=clean-%-mimiker-elf) clean-host
	$(RM) -r $(SRCDIR)

.PHONY: all build clean build-host clean-host

### GNU Binutils

$(SRCDIR)/$(BINUTILS).tar.xz:
	$(CURL) -o $@ $(BINUTILS-URL)

binutils-unpack: $(SRCDIR)/binutils/.unpack
$(SRCDIR)/binutils/.unpack: $(SRCDIR)/$(BINUTILS).tar.xz
	$(CD) $(SRCDIR) && $(TAR) xJf $^ && $(MV) $(BINUTILS) binutils
	$(TOUCH) $@

.PHONY: binutils-unpack

### GNU Compiler Collection

$(SRCDIR)/$(GCC).tar.xz:
	$(CURL) -o $@ $(GCC-URL)

gcc-unpack: $(SRCDIR)/gcc/.unpack
$(SRCDIR)/gcc/.unpack: $(SRCDIR)/$(GCC).tar.xz
	$(CD) $(SRCDIR) && $(TAR) xJf $^ && $(MV) $(GCC) gcc
	$(TOUCH) $@

.PHONY: gcc-unpack

### GNU Debugger

$(SRCDIR)/$(GDB).tar.xz:
	$(CURL) -o $@ $(GDB-URL)

gdb-unpack: $(SRCDIR)/gdb/.unpack
$(SRCDIR)/gdb/.unpack: $(SRCDIR)/$(GDB).tar.xz
	$(CD) $(SRCDIR) && $(TAR) xJf $^ && $(MV) $(GDB) gdb
	$(TOUCH) $@

.PHONY: gdb-unpack

### The GNU Multiple Precision Arithmetic Library

$(SRCDIR)/$(GMP).tar.xz:
	$(CURL) -o $@ $(GMP-URL)

gmp-unpack: $(SRCDIR)/gmp/.unpack
$(SRCDIR)/gmp/.unpack: $(SRCDIR)/$(GMP).tar.xz
	$(CD) $(SRCDIR) && $(TAR) xJf $^ && $(MV) $(GMP) gmp
	$(TOUCH) $@

gmp-configure: $(HOSTBUILD)/gmp/.configure
$(HOSTBUILD)/gmp/.configure: $(SRCDIR)/gmp/.unpack
	$(MKDIR) $(HOSTBUILD)/gmp && $(CD) $(HOSTBUILD)/gmp && $(SRCDIR)/gmp/configure \
		--disable-shared \
		--prefix=$(HOSTDIR)
	$(TOUCH) $@

gmp-build: $(HOSTBUILD)/gmp/.build
$(HOSTBUILD)/gmp/.build: $(HOSTBUILD)/gmp/.configure
	$(CD) $(HOSTBUILD)/gmp && $(MAKE)
	$(TOUCH) $@

gmp-install: $(HOSTBUILD)/gmp/.install
$(HOSTBUILD)/gmp/.install: $(HOSTBUILD)/gmp/.build
	$(CD) $(HOSTBUILD)/gmp && $(MAKE) install-strip
	$(TOUCH) $@

gmp-clean:
	$(RM) -r $(HOSTBUILD)/gmp

### Library for multiple-precision floating-point computations

$(SRCDIR)/$(MPFR).tar.xz:
	$(CURL) -o $@ $(MPFR-URL)

mpfr-unpack: $(SRCDIR)/mpfr/.unpack
$(SRCDIR)/mpfr/.unpack: $(SRCDIR)/$(MPFR).tar.xz
	$(CD) $(SRCDIR) && $(TAR) xJf $^ && $(MV) $(MPFR) mpfr
	$(TOUCH) $@

mpfr-configure: mpfr/.configure
$(HOSTBUILD)/mpfr/.configure: $(SRCDIR)/mpfr/.unpack $(HOSTBUILD)/gmp/.install
	$(MKDIR) $(HOSTBUILD)/mpfr && $(CD) $(HOSTBUILD)/mpfr && $(SRCDIR)/mpfr/configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp=$(HOSTDIR)
	$(TOUCH) $@

mpfr-build: $(HOSTBUILD)/mpfr/.build
$(HOSTBUILD)/mpfr/.build: $(HOSTBUILD)/mpfr/.configure
	$(CD) $(HOSTBUILD)/mpfr && $(MAKE)
	$(TOUCH) $@

mpfr-install: $(HOSTBUILD)/mpfr/.install
$(HOSTBUILD)/mpfr/.install: $(HOSTBUILD)/mpfr/.build
	$(CD) $(HOSTBUILD)/mpfr && $(MAKE) install-strip
	$(TOUCH) $@

mpfr-clean:
	$(RM) -r $(HOSTBUILD)/mpfr

### Library for the arithmetic of complex numbers

$(SRCDIR)/$(MPC).tar.gz:
	$(CURL) -o $@ $(MPC-URL)

mpc-unpack: $(SRCDIR)/mpc/.unpack
$(SRCDIR)/mpc/.unpack: $(SRCDIR)/$(MPC).tar.gz
	$(CD) $(SRCDIR) && $(TAR) xzf $^ && $(MV) $(MPC) mpc
	$(TOUCH) $@

mpc-configure: $(HOSTBUILD)/mpc/.configure
$(HOSTBUILD)/mpc/.configure: $(SRCDIR)/mpc/.unpack $(HOSTBUILD)/gmp/.install $(HOSTBUILD)/mpfr/.install
	$(MKDIR) $(HOSTBUILD)/mpc && $(CD) $(HOSTBUILD)/mpc && $(SRCDIR)/mpc/configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp=$(HOSTDIR) \
		--with-mpfr=$(HOSTDIR)
	$(TOUCH) $@

mpc-build: $(HOSTBUILD)/mpc/.build
$(HOSTBUILD)/mpc/.build: $(HOSTBUILD)/mpc/.configure
	$(CD) $(HOSTBUILD)/mpc && $(MAKE)
	$(TOUCH) $@

mpc-install: $(HOSTBUILD)/mpc/.install
$(HOSTBUILD)/mpc/.install: $(HOSTBUILD)/mpc/.build
	$(CD) $(HOSTBUILD)/mpc && $(MAKE) install-strip
	$(TOUCH) $@

mpc-clean:
	$(RM) -r $(HOSTBUILD)/mpc

### Integer Set Library

$(SRCDIR)/$(ISL).tar.xz:
	$(CURL) -o $@ $(ISL-URL)

isl-unpack: $(SRCDIR)/isl/.unpack
$(SRCDIR)/isl/.unpack: $(SRCDIR)/$(ISL).tar.xz
	$(CD) $(SRCDIR) && $(TAR) xf $^ && $(MV)  $(ISL) isl
	$(TOUCH) $@

isl-configure: $(HOSTBUILD)/isl/.configure
$(HOSTBUILD)/isl/.configure: $(SRCDIR)/isl/.unpack $(HOSTBUILD)/gmp/.install
	$(MKDIR) $(HOSTBUILD)/isl && $(CD) $(HOSTBUILD)/isl && $(SRCDIR)/isl/configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp-prefix=$(HOSTDIR)
	$(TOUCH) $@

isl-build: $(HOSTBUILD)/isl/.build
$(HOSTBUILD)/isl/.build: $(HOSTBUILD)/isl/.configure
	$(CD) $(HOSTBUILD)/isl && $(MAKE)
	$(TOUCH) $@

isl-install: $(HOSTBUILD)/isl/.install
$(HOSTBUILD)/isl/.install: $(HOSTBUILD)/isl/.build
	$(CD) $(HOSTBUILD)/isl && $(MAKE) install-strip
	$(TOUCH) $@

isl-clean:
	$(RM) -r $(HOSTBUILD)/isl

### CLooG - The Chunky Loop Generator

$(SRCDIR)/$(CLOOG).tar.gz:
	$(CURL) -o $@ $(CLOOG-URL)

cloog-unpack: $(SRCDIR)/cloog/.unpack
$(SRCDIR)/cloog/.unpack: $(SRCDIR)/$(CLOOG).tar.gz
	$(CD) $(SRCDIR) && $(TAR) xzf $^ && $(MV)  $(CLOOG) cloog
	$(TOUCH) $@

cloog-configure: $(HOSTBUILD)/cloog/.configure
$(HOSTBUILD)/cloog/.configure: $(SRCDIR)/cloog/.unpack $(HOSTBUILD)/gmp/.install $(HOSTBUILD)/isl/.install
	$(MKDIR) $(HOSTBUILD)/cloog && $(CD) $(HOSTBUILD)/cloog && $(SRCDIR)/cloog/configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-isl=system \
		--with-gmp-prefix=$(HOSTDIR) \
		--with-isl-prefix=$(HOSTDIR)
	$(TOUCH) $@

cloog-build: $(HOSTBUILD)/cloog/.build
$(HOSTBUILD)/cloog/.build: $(HOSTBUILD)/cloog/.configure
	$(CD) $(HOSTBUILD)/cloog && $(MAKE)
	$(TOUCH) $@

cloog-install: $(HOSTBUILD)/cloog/.install
$(HOSTBUILD)/cloog/.install: $(HOSTBUILD)/cloog/.build
	$(CD) $(HOSTBUILD)/cloog && $(MAKE) install-strip
	$(TOUCH) $@

cloog-clean:
	$(RM) -r $(HOSTBUILD)/cloog


.PHONY: $(foreach util, isl mpfr gmp mpc cloog, $(foreach op, unpack configure build install clean, $(util)-$(op)))
