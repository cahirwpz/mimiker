CURL = curl -L
RM = rm -f

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

HOSTDIR := $(ROOTDIR)/host
PREFIX ?= $(ROOTDIR)/local

all: gcc-install gdb-install

clean: gmp-clean mpfr-clean mpc-clean isl-clean cloog-clean binutils-clean gcc-clean gdb-clean
	$(RM) -r host
	$(RM) *~

unpack: gmp-unpack mpfr-unpack mpc-unpack isl-unpack cloog-clean binutils-unpack gcc-unpack gdb-unpack

.PHONY: all clean download

### The GNU Multiple Precision Arithmetic Library

$(GMP).tar.xz:
	$(CURL) -o $@ "https://gmplib.org/download/gmp/$(GMP).tar.xz"

gmp-unpack: gmp/.unpack
gmp/.unpack: $(GMP).tar.xz
	tar xJf $^
	mv $(GMP) gmp
	touch $@

gmp-configure: gmp/.configure
gmp/.configure: gmp/.unpack
	cd gmp && ./configure \
		--disable-shared \
		--prefix=$(HOSTDIR)
	touch $@

gmp-build: gmp/.build
gmp/.build: gmp/.configure
	cd gmp && $(MAKE)
	touch $@

gmp-install: gmp/.install
gmp/.install: gmp/.build
	cd gmp && $(MAKE) install-strip
	touch $@

gmp-clean:
	$(RM) -r gmp $(GMP).tar.xz

.PHONY: gmp-unpack gmp-configure gmp-build gmp-install gmp-clean

### Library for multiple-precision floating-point computations

$(MPFR).tar.xz:
	$(CURL) -o $@ "ftp://ftp.gnu.org/gnu/mpfr/$(MPFR).tar.xz"

mpfr-unpack: mpfr/.unpack
mpfr/.unpack: $(MPFR).tar.xz
	tar xJf $^
	mv $(MPFR) mpfr
	touch $@

mpfr-configure: mpfr/.configure
mpfr/.configure: mpfr/.unpack gmp/.install
	cd mpfr && ./configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp=$(HOSTDIR)
	touch $@

mpfr-build: mpfr/.build
mpfr/.build: mpfr/.configure
	cd mpfr && $(MAKE)
	touch $@

mpfr-install: mpfr/.install
mpfr/.install: mpfr/.build
	cd mpfr && $(MAKE) install-strip
	touch $@

mpfr-clean:
	$(RM) -r mpfr $(MPFR).tar.xz

.PHONY: mpfr-unpack mpfr-configure mpfr-build mpfr-install mpfr-clean

### Library for the arithmetic of complex numbers

$(MPC).tar.gz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/mpc/$(MPC).tar.gz"

mpc-unpack: mpc/.unpack
mpc/.unpack: $(MPC).tar.gz
	tar xzf $^
	mv $(MPC) mpc
	touch $@

mpc-configure: mpc/.configure
mpc/.configure: mpc/.unpack gmp/.install mpfr/.install
	cd mpc && ./configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp=$(HOSTDIR) \
		--with-mpfr=$(HOSTDIR)
	touch $@

mpc-build: mpc/.build
mpc/.build: mpc/.configure
	cd mpc && $(MAKE)
	touch $@

mpc-install: mpc/.install
mpc/.install: mpc/.build
	cd mpc && $(MAKE) install-strip
	touch $@

mpc-clean:
	$(RM) -r mpc $(MPC).tar.gz

.PHONY: mpc-unpack mpc-configure mpc-build mpc-install mpc-clean

### Integer Set Library

$(ISL).tar.xz:
	$(CURL) -o $@ "http://isl.gforge.inria.fr/$(ISL).tar.xz"

isl-unpack: isl/.unpack
isl/.unpack: $(ISL).tar.xz
	tar xJf $^
	mv $(ISL) isl
	touch $@

isl-configure: isl/.configure
isl/.configure: isl/.unpack gmp/.install
	cd isl && ./configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-gmp-prefix=$(HOSTDIR)
	touch $@

isl-build: isl/.build
isl/.build: isl/.configure
	cd isl && $(MAKE)
	touch $@

isl-install: isl/.install
isl/.install: isl/.build
	cd isl && $(MAKE) install-strip
	touch $@

isl-clean:
	$(RM) -r isl $(ISL).tar.xz

.PHONY: isl-unpack isl-configure isl-build isl-install isl-clean

### CLooG - The Chunky Loop Generator

$(CLOOG).tar.gz:
	$(CURL) -o $@ "http://www.bastoul.net/cloog/pages/download/$(CLOOG).tar.gz"

cloog-unpack: cloog/.unpack
cloog/.unpack: $(CLOOG).tar.gz
	tar xzf $^
	mv $(CLOOG) cloog
	touch $@

cloog-configure: cloog/.configure
cloog/.configure: cloog/.unpack gmp/.install isl/.install
	cd cloog && ./configure \
		--disable-shared \
		--prefix=$(HOSTDIR) \
		--with-isl=system \
		--with-gmp-prefix=$(HOSTDIR) \
		--with-isl-prefix=$(HOSTDIR)
	touch $@

cloog-build: cloog/.build
cloog/.build: cloog/.configure
	cd cloog && $(MAKE)
	touch $@

cloog-install: cloog/.install
cloog/.install: cloog/.build
	cd cloog && $(MAKE) install-strip
	touch $@

cloog-clean:
	$(RM) -r cloog $(CLOOG).tar.gz

.PHONY: cloog-unpack cloog-configure cloog-build cloog-install cloog-clean

### List of all requisite libraries

LIBRARIES = gmp/.install mpfr/.install mpc/.install isl/.install cloog/.install

### GNU Binutils

$(BINUTILS).tar.xz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/binutils/$(BINUTILS).tar.xz"

binutils-unpack: binutils/.unpack
binutils/.unpack: $(BINUTILS).tar.xz
	tar xJf $^
	mv $(BINUTILS) binutils
	touch $@

binutils-configure: binutils/.configure
binutils/.configure: binutils/.unpack $(LIBRARIES)
	cd binutils && ./configure \
		--target=$(TARGET) \
		--prefix=$(PREFIX) \
		--datarootdir=$(PREFIX)/$(TARGET)/share \
		--with-sysroot=$(PREFIX)/$(TARGET) \
		--disable-nls \
		--disable-shared \
		--disable-multilib \
		--disable-werror \
		--with-gmp=$(HOSTDIR) \
		--with-mpfr=$(HOSTDIR) \
		--with-mpc=$(HOSTDIR) \
		--with-isl=$(HOSTDIR) \
		--with-python=$(shell which python3)
	touch $@

binutils-build: binutils/.build
binutils/.build: binutils/.configure
	cd binutils && $(MAKE)
	touch $@

binutils-install: binutils/.install
binutils/.install: binutils/.build
	cd binutils && $(MAKE) install-strip
	touch $@

binutils-clean:
	$(RM) -r binutils $(BINUTILS).tar.xz

.PHONY: binutils-unpack binutils-configure binutils-build binutils-install binutils-clean

### GNU Compiler Collection

$(GCC).tar.xz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/gcc/$(GCC)/$(GCC).tar.xz"

gcc-unpack: gcc/.unpack
gcc/.unpack: $(GCC).tar.xz
	tar xJf $^
	mv $(GCC) gcc
	touch $@

gcc-configure: gcc/.configure
gcc/.configure: gcc/.unpack binutils/.install $(LIBRARIES)
	echo $(TARGET)
	echo $(GCC_EXTRA_CONF)
	cd gcc && PATH=$(PREFIX)/bin:$$PATH ./configure \
		--target=$(TARGET) \
		--prefix=$(PREFIX) \
		--datarootdir=$(PREFIX)/$(TARGET)/share \
		--libexecdir=$(PREFIX)/$(TARGET)/libexec \
		--with-sysroot=$(PREFIX)/$(TARGET)/sysroot \
		--with-gmp=$(HOSTDIR) \
		--with-mpfr=$(HOSTDIR) \
		--with-mpc=$(HOSTDIR) \
		--with-isl=$(HOSTDIR) \
		--with-cloog=$(HOSTDIR) \
		--enable-languages=c \
		--enable-lto \
		--disable-multilib \
		--disable-nls \
		--disable-shared \
		--disable-werror \
		--with-newlib \
		--without-headers \
		$(GCC_EXTRA_CONF)
	touch $@

gcc-build: gcc/.build
gcc/.build: gcc/.configure
	cd gcc && $(MAKE) all-gcc
	touch $@

gcc-install: gcc/.install
gcc/.install: gcc/.build
	cd gcc && $(MAKE) install-strip-gcc
	touch $@

gcc-clean:
	$(RM) -r gcc $(GCC).tar.xz

.PHONY: gcc-unpack gcc-configure gcc-build gcc-install gcc-clean

### GNU Debugger

$(GDB).tar.xz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/gdb/$(GDB).tar.xz"

gdb-unpack: gdb/.unpack
gdb/.unpack: $(GDB).tar.xz
	tar xJf $^
	mv $(GDB) gdb
	touch $@

gdb-configure: gdb/.configure
gdb/.configure: gdb/.unpack gcc/.install $(LIBRARIES)
	cd gdb && PATH=$(PREFIX)/bin:$$PATH ./configure \
		--target=$(TARGET)\
		--prefix=$(PREFIX) \
		--datarootdir=$(PREFIX)/$(TARGET)/share \
		--with-sysroot=$(PREFIX)/$(TARGET) \
		--with-isl=$(HOSTDIR) \
		--disable-binutils \
		--disable-gas \
		--disable-ld \
		--disable-nls \
		--disable-sim \
		--disable-werror \
		--disable-source-highlight \
		--with-tui \
		--with-python=$(shell which python3)
	touch $@

gdb-build: gdb/.build
gdb/.build: gdb/.configure
	cd gdb && $(MAKE)
	touch $@

gdb-install: gdb/.install
gdb/.install: gdb/.build
	cd gdb && $(MAKE) install-strip-gdb
	touch $@

gdb-clean:
	$(RM) -r gdb $(GDB).tar.xz

.PHONY: gdb-unpack gdb-configure gdb-build gdb-install gdb-clean
