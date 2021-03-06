include $(TOPDIR)/build/common.mk

DESTDIR := $(TOPDIR)/$(TARGET)/debian/tmp

all: gcc-install gdb-install

clean: binutils-clean gcc-clean gdb-clean
	$(RM) -r debian

package:
	$(CP) -T -a $(TOPDIR)/debian debian
	sed -i -e 's#%{TARGET}#$(TARGET:%-mimiker-elf=%)#g' \
	       -e 's#%{DATE}#$(shell date -R)#g' \
	       -e 's#%{VERSION}#$(VERSION)#g' \
	       `find debian -type f`
	fakeroot ./debian/rules binary

.PHONY: all clean package

### GNU Binutils

$(SRCDIR)/$(BINUTILS).tar.xz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/binutils/$(BINUTILS).tar.xz"

binutils-unpack: $(SRCDIR)/binutils/.unpack
$(SRCDIR)/binutils/.unpack: $(SRCDIR)/$(BINUTILS).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(BINUTILS) binutils
	touch $@

binutils-configure: binutils/.configure
binutils/.configure: $(SRCDIR)/binutils/.unpack
	mkdir -p binutils && cd binutils && $(SRCDIR)/binutils/configure \
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
	cd binutils && $(MAKE) install DESTDIR=$(DESTDIR)
	touch $@

binutils-clean:
	$(RM) -r binutils $(SRCDIR)/$(BINUTILS).tar.xz $(SRCDIR)/binutils

.PHONY: binutils-unpack binutils-configure binutils-build binutils-install binutils-clean

### GNU Compiler Collection

$(SRCDIR)/$(GCC).tar.xz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/gcc/$(GCC)/$(GCC).tar.xz"

gcc-unpack: $(SRCDIR)/gcc/.unpack
$(SRCDIR)/gcc/.unpack: $(SRCDIR)/$(GCC).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(GCC) gcc
	touch $@

gcc-configure: gcc/.configure
gcc/.configure: $(SRCDIR)/gcc/.unpack binutils/.install
	mkdir -p gcc && cd gcc && PATH=$(PREFIX)/bin:$$PATH $(SRCDIR)/gcc/configure \
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
	cd gcc && $(MAKE) all-target-gcc
	touch $@

gcc-install: gcc/.install
gcc/.install: gcc/.build
	cd gcc && $(MAKE) install-gcc DESTDIR=$(DESTDIR)
	cd gcc && $(MAKE) install-target-libgcc DESTDIR=$(DESTDIR)
	touch $@

gcc-clean:
	$(RM) -r gcc $(SRCDIR)/$(GCC).tar.xz $(SRCDIR)/gcc

.PHONY: gcc-unpack gcc-configure gcc-build gcc-install gcc-clean

### GNU Debugger

$(SRCDIR)/$(GDB).tar.xz:
	$(CURL) -o $@ "https://ftp.gnu.org/gnu/gdb/$(GDB).tar.xz"

gdb-unpack: $(SRCDIR)/gdb/.unpack
$(SRCDIR)/gdb/.unpack: $(SRCDIR)/$(GDB).tar.xz
	cd $(SRCDIR) && tar xJf $^ && mv $(GDB) gdb
	touch $@

gdb-configure: gdb/.configure
gdb/.configure: $(SRCDIR)/gdb/.unpack gcc/.install $(LIBRARIES)
	mkdir -p gdb && cd gdb && PATH=$(PREFIX)/bin:$$PATH $(SRCDIR)/gdb/configure \
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
	cd gdb && $(MAKE) install-gdb DESTDIR=$(DESTDIR)
	touch $@

gdb-clean:
	$(RM) -r gdb $(SRCDIR)/$(GDB).tar.xz $(SRCDIR)/gdb

.PHONY: gdb-unpack gdb-configure gdb-build gdb-install gdb-clean
