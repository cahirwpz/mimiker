TOPDIR = $(CURDIR)

include $(TOPDIR)/common.mk

all: package

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
	$(RM) -r sources

.PHONY: all build clean build-host clean-host

### GNU Binutils

$(SRCDIR)/$(BINUTILS).tar.xz:
	$(CURL) -o $@ $(BINUTILS-URL)

binutils-unpack: $(SRCDIR)/binutils/.unpack
$(SRCDIR)/binutils/.unpack: $(SRCDIR)/$(BINUTILS).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(BINUTILS) binutils
	touch $@

.PHONY: binutils-unpack

### GNU Compiler Collection

$(SRCDIR)/$(GCC).tar.xz:
	$(CURL) -o $@ $(GCC-URL)

gcc-unpack: $(SRCDIR)/gcc/.unpack
$(SRCDIR)/gcc/.unpack: $(SRCDIR)/$(GCC).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(GCC) gcc
	touch $@

.PHONY: gcc-unpack

### GNU Debugger

$(SRCDIR)/$(GDB).tar.xz:
	$(CURL) -o $@ $(GDB-URL)

gdb-unpack: $(SRCDIR)/gdb/.unpack
$(SRCDIR)/gdb/.unpack: $(SRCDIR)/$(GDB).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(GDB) gdb
	touch $@

.PHONY: gdb-unpack

### The GNU Multiple Precision Arithmetic Library

$(SRCDIR)/$(GMP).tar.xz:
	$(CURL) -o $@ $(GMP-URL)

gmp-unpack: $(SRCDIR)/gmp/.unpack
$(SRCDIR)/gmp/.unpack: $(SRCDIR)/$(GMP).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(GMP) gmp
	touch $@

gmp-configure: $(HOSTBUILD)/gmp/.configure
$(HOSTBUILD)/gmp/.configure: $(SRCDIR)/gmp/.unpack
	mkdir -p $(HOSTBUILD)/gmp && cd $(HOSTBUILD)/gmp && $(SRCDIR)/gmp/configure \
		--disable-shared \
		--prefix=$(HOSTDIR)
	touch $@

gmp-build: $(HOSTBUILD)/gmp/.build
$(HOSTBUILD)/gmp/.build: $(HOSTBUILD)/gmp/.configure
	cd $(HOSTBUILD)/gmp && $(MAKE)
	touch $@

gmp-install: $(HOSTBUILD)/gmp/.install
$(HOSTBUILD)/gmp/.install: $(HOSTBUILD)/gmp/.build
	cd $(HOSTBUILD)/gmp && $(MAKE) install-strip
	touch $@

gmp-clean:
	$(RM) -r $(HOSTBUILD)/gmp

.PHONY: gmp-unpack gmp-configure gmp-build gmp-install gmp-clean

### Library for multiple-precision floating-point computations

$(SRCDIR)/$(MPFR).tar.xz:
	$(CURL) -o $@ $(MPFR-URL)

mpfr-unpack: $(SRCDIR)/mpfr/.unpack
$(SRCDIR)/mpfr/.unpack: $(SRCDIR)/$(MPFR).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(MPFR) mpfr
	touch $@

mpfr-configure: mpfr/.configure
$(HOSTBUILD)/mpfr/.configure: $(SRCDIR)/mpfr/.unpack $(HOSTBUILD)/gmp/.install
	mkdir -p $(HOSTBUILD)/mpfr && cd $(HOSTBUILD)/mpfr && $(SRCDIR)/mpfr/configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp=$(HOSTDIR)
	touch $@

mpfr-build: $(HOSTBUILD)/mpfr/.build
$(HOSTBUILD)/mpfr/.build: $(HOSTBUILD)/mpfr/.configure
	cd $(HOSTBUILD)/mpfr && $(MAKE)
	touch $@

mpfr-install: $(HOSTBUILD)/mpfr/.install
$(HOSTBUILD)/mpfr/.install: $(HOSTBUILD)/mpfr/.build
	cd $(HOSTBUILD)/mpfr && $(MAKE) install-strip
	touch $@

mpfr-clean:
	$(RM) -r $(HOSTBUILD)/mpfr

.PHONY: mpfr-unpack mpfr-configure mpfr-build mpfr-install mpfr-clean

### Library for the arithmetic of complex numbers

$(SRCDIR)/$(MPC).tar.gz:
	$(CURL) -o $@ $(MPC-URL)

mpc-unpack: $(SRCDIR)/mpc/.unpack
$(SRCDIR)/mpc/.unpack: $(SRCDIR)/$(MPC).tar.gz
	cd $(SRCDIR) && tar xzf $^ && mv $(MPC) mpc
	touch $@

mpc-configure: $(HOSTBUILD)/mpc/.configure
$(HOSTBUILD)/mpc/.configure: $(SRCDIR)/mpc/.unpack $(HOSTBUILD)/gmp/.install $(HOSTBUILD)/mpfr/.install
	mkdir -p $(HOSTBUILD)/mpc && cd $(HOSTBUILD)/mpc && $(SRCDIR)/mpc/configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp=$(HOSTDIR) \
		--with-mpfr=$(HOSTDIR)
	touch $@

mpc-build: $(HOSTBUILD)/mpc/.build
$(HOSTBUILD)/mpc/.build: $(HOSTBUILD)/mpc/.configure
	cd $(HOSTBUILD)/mpc && $(MAKE)
	touch $@

mpc-install: $(HOSTBUILD)/mpc/.install
$(HOSTBUILD)/mpc/.install: $(HOSTBUILD)/mpc/.build
	cd $(HOSTBUILD)/mpc && $(MAKE) install-strip
	touch $@

mpc-clean:
	$(RM) -r $(HOSTBUILD)/mpc

.PHONY: mpc-unpack mpc-configure mpc-build mpc-install mpc-clean

### Integer Set Library

$(SRCDIR)/$(ISL).tar.xz:
	$(CURL) -o $@ $(ISL-URL)

isl-unpack: $(SRCDIR)/isl/.unpack
$(SRCDIR)/isl/.unpack: $(SRCDIR)/$(ISL).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(ISL) isl
	touch $@

isl-configure: $(HOSTBUILD)/isl/.configure
$(HOSTBUILD)/isl/.configure: $(SRCDIR)/isl/.unpack $(HOSTBUILD)/gmp/.install
	mkdir -p $(HOSTBUILD)/isl && cd $(HOSTBUILD)/isl && $(SRCDIR)/isl/configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp-prefix=$(HOSTDIR)
	touch $@

isl-build: $(HOSTBUILD)/isl/.build
$(HOSTBUILD)/isl/.build: $(HOSTBUILD)/isl/.configure
	cd $(HOSTBUILD)/isl && $(MAKE)
	touch $@

isl-install: $(HOSTBUILD)/isl/.install
$(HOSTBUILD)/isl/.install: $(HOSTBUILD)/isl/.build
	cd $(HOSTBUILD)/isl && $(MAKE) install-strip
	touch $@

isl-clean:
	$(RM) -r $(HOSTBUILD)/isl

.PHONY: isl-unpack isl-configure isl-build isl-install isl-clean

### CLooG - The Chunky Loop Generator

$(SRCDIR)/$(CLOOG).tar.gz:
	$(CURL) -o $@ $(CLOOG-URL)

cloog-unpack: $(SRCDIR)/cloog/.unpack
$(SRCDIR)/cloog/.unpack: $(SRCDIR)/$(CLOOG).tar.gz
	cd $(SRCDIR) && tar xzf $^ && mv $(CLOOG) cloog
	touch $@

cloog-configure: $(HOSTBUILD)/cloog/.configure
$(HOSTBUILD)/cloog/.configure: $(SRCDIR)/cloog/.unpack $(HOSTBUILD)/gmp/.install $(HOSTBUILD)/isl/.install
	mkdir -p $(HOSTBUILD)/cloog && cd $(HOSTBUILD)/cloog && $(SRCDIR)/cloog/configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-isl=system \
		--with-gmp-prefix=$(HOSTDIR) \
		--with-isl-prefix=$(HOSTDIR)
	touch $@

cloog-build: $(HOSTBUILD)/cloog/.build
$(HOSTBUILD)/cloog/.build: $(HOSTBUILD)/cloog/.configure
	cd $(HOSTBUILD)/cloog && $(MAKE)
	touch $@

cloog-install: $(HOSTBUILD)/cloog/.install
$(HOSTBUILD)/cloog/.install: $(HOSTBUILD)/cloog/.build
	cd $(HOSTBUILD)/cloog && $(MAKE) install-strip
	touch $@

cloog-clean:
	$(RM) -r $(HOSTBUILD)/cloog

.PHONY: cloog-unpack cloog-configure cloog-build cloog-install cloog-clean
