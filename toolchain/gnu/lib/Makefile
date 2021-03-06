TOPDIR = $(realpath ..)

include $(TOPDIR)/build/common.mk

all: gmp-install mpfr-install mpc-install isl-install cloog-install

clean: gmp-clean mpfr-clean mpc-clean isl-clean cloog-clean
	$(RM) -r $(HOSTDIR)

.PHONY: all clean

### The GNU Multiple Precision Arithmetic Library

$(SRCDIR)/$(GMP).tar.xz:
	$(CURL) -o $@ "https://gmplib.org/download/gmp/$(GMP).tar.xz"

gmp-unpack: $(SRCDIR)/gmp/.unpack
$(SRCDIR)/gmp/.unpack: $(SRCDIR)/$(GMP).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(GMP) gmp
	touch $@

gmp-configure: gmp/.configure
gmp/.configure: $(SRCDIR)/gmp/.unpack
	mkdir -p gmp && cd gmp && $(SRCDIR)/gmp/configure \
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
	$(RM) -r gmp $(SRCDIR)/$(GMP).tar.xz $(SRCDIR)/gmp

.PHONY: gmp-unpack gmp-configure gmp-build gmp-install gmp-clean

### Library for multiple-precision floating-point computations

$(SRCDIR)/$(MPFR).tar.xz:
	$(CURL) -o $@ "ftp://ftp.gnu.org/gnu/mpfr/$(MPFR).tar.xz"

mpfr-unpack: $(SRCDIR)/mpfr/.unpack
$(SRCDIR)/mpfr/.unpack: $(SRCDIR)/$(MPFR).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(MPFR) mpfr
	touch $@

mpfr-configure: mpfr/.configure
mpfr/.configure: $(SRCDIR)/mpfr/.unpack gmp/.install
	mkdir -p mpfr && cd mpfr && $(SRCDIR)/mpfr/configure \
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
	$(RM) -r mpfr $(SRCDIR)/$(MPFR).tar.xz $(SRCDIR)/mpfr

.PHONY: mpfr-unpack mpfr-configure mpfr-build mpfr-install mpfr-clean

### Library for the arithmetic of complex numbers

$(SRCDIR)/$(MPC).tar.gz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/mpc/$(MPC).tar.gz"

mpc-unpack: $(SRCDIR)/mpc/.unpack
$(SRCDIR)/mpc/.unpack: $(SRCDIR)/$(MPC).tar.gz
	cd $(SRCDIR) && tar xzf $^ && mv $(MPC) mpc
	touch $@

mpc-configure: mpc/.configure
mpc/.configure: $(SRCDIR)/mpc/.unpack gmp/.install mpfr/.install
	mkdir -p mpc && cd mpc && $(SRCDIR)/mpc/configure \
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
	$(RM) -r mpc $(SRCDIR)/$(MPC).tar.gz $(SRCDIR)/mpc

.PHONY: mpc-unpack mpc-configure mpc-build mpc-install mpc-clean

### Integer Set Library

$(SRCDIR)/$(ISL).tar.xz:
	$(CURL) -o $@ "http://isl.gforge.inria.fr/$(ISL).tar.xz"

isl-unpack: $(SRCDIR)/isl/.unpack
$(SRCDIR)/isl/.unpack: $(SRCDIR)/$(ISL).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(ISL) isl
	touch $@

isl-configure: isl/.configure
isl/.configure: $(SRCDIR)/isl/.unpack gmp/.install
	mkdir -p isl && cd isl && $(SRCDIR)/isl/configure \
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
	$(RM) -r isl $(SRCDIR)/$(ISL).tar.xz $(SRCDIR)/isl

.PHONY: isl-unpack isl-configure isl-build isl-install isl-clean

### CLooG - The Chunky Loop Generator

$(SRCDIR)/$(CLOOG).tar.gz:
	$(CURL) -o $@ "http://www.bastoul.net/cloog/pages/download/$(CLOOG).tar.gz"

cloog-unpack: $(SRCDIR)/cloog/.unpack
$(SRCDIR)/cloog/.unpack: $(SRCDIR)/$(CLOOG).tar.gz
	cd $(SRCDIR) && tar xzf $^ && mv $(CLOOG) cloog
	touch $@

cloog-configure: cloog/.configure
cloog/.configure: $(SRCDIR)/cloog/.unpack gmp/.install isl/.install
	mkdir -p cloog && cd cloog && $(SRCDIR)/cloog/configure \
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
	$(RM) -r cloog $(SRCDIR)/$(CLOOG).tar.gz $(SRCDIR)/cloog

.PHONY: cloog-unpack cloog-configure cloog-build cloog-install cloog-clean
