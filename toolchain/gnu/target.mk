include $(TOPDIR)/common.mk

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

binutils-configure: binutils/.configure
binutils/.configure:
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
	cd binutils && $(MAKE) LDFLAGS=-Wl,--as-needed
	touch $@

binutils-install: binutils/.install
binutils/.install: binutils/.build
	cd binutils && $(MAKE) install DESTDIR=$(DESTDIR)
	touch $@

binutils-clean:
	$(RM) -r binutils

.PHONY: binutils-configure binutils-build binutils-install binutils-clean

### GNU Compiler Collection

TARGET_TOOLS = AS_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-as \
		 AR_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-ar \
		 LD_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-ld \
		 NM_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-nm \
		 STRIP_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-strip \
		 RANLIB_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-ranlib \

gcc-configure: gcc/.configure
gcc/.configure: binutils/.install
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
	cd gcc && $(MAKE) all-gcc $(TARGET_TOOLS)
	cd gcc && $(MAKE) all-target-libgcc $(TARGET_TOOLS)
	touch $@

gcc-install: gcc/.install
gcc/.install: gcc/.build
	cd gcc && $(MAKE) install-gcc DESTDIR=$(DESTDIR) $(TARGET_TOOLS)
	cd gcc && $(MAKE) install-target-libgcc DESTDIR=$(DESTDIR) $(TARGET_TOOLS)
	touch $@

gcc-clean:
	$(RM) -r gcc

.PHONY: gcc-configure gcc-build gcc-install gcc-clean

### GNU Debugger

gdb-configure: gdb/.configure
gdb/.configure: gcc/.install
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
	cd gdb && $(MAKE) LDFLAGS=-Wl,--as-needed
	touch $@

gdb-install: gdb/.install
gdb/.install: gdb/.build
	cd gdb && $(MAKE) install-gdb DESTDIR=$(DESTDIR)
	touch $@

gdb-clean:
	$(RM) -r gdb

.PHONY: gdb-configure gdb-build gdb-install gdb-clean
