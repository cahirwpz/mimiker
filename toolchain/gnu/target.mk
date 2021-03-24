# vim: tabstop=8 shiftwidth=8 noexpandtab list:

all: gcc-install gdb-install

include $(TOPDIR)/common.mk

DESTDIR := $(CURDIR)/debian/tmp

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
	$(MKDIR) binutils && $(CD) binutils && $(SRCDIR)/binutils/configure \
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
	$(TOUCH) $@

binutils-build: binutils/.build
binutils/.build: binutils/.configure
	$(CD) binutils && $(MAKE) LDFLAGS=-Wl,--as-needed
	$(TOUCH) $@

binutils-install: binutils/.install
binutils/.install: binutils/.build
	$(CD) binutils && $(MAKE) install DESTDIR=$(DESTDIR)
	$(TOUCH) $@

binutils-clean:
	$(RM) -r binutils

### GNU Compiler Collection

TARGET_TOOLS = AS_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-as \
	       AR_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-ar \
	       LD_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-ld \
	       NM_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-nm \
	       STRIP_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-strip \
	       RANLIB_FOR_TARGET=$(DESTDIR)$(PREFIX)/bin/$(TARGET)-ranlib \

gcc-configure: gcc/.configure
gcc/.configure: binutils/.install
	$(MKDIR) gcc && $(CD) gcc && PATH=$(DESTDIR)$(PREFIX)/bin:$$PATH $(SRCDIR)/gcc/configure \
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
	$(TOUCH) $@

gcc-build: gcc/.build
gcc/.build: gcc/.configure
	$(CD) gcc && $(MAKE) all-gcc $(TARGET_TOOLS)
	$(CD) gcc && $(MAKE) all-target-libgcc $(TARGET_TOOLS)
	$(TOUCH) $@

gcc-install: gcc/.install
gcc/.install: gcc/.build
	$(CD) gcc && $(MAKE) install-gcc DESTDIR=$(DESTDIR) $(TARGET_TOOLS)
	$(CD) gcc && $(MAKE) install-target-libgcc DESTDIR=$(DESTDIR) $(TARGET_TOOLS)
	$(TOUCH) $@

gcc-clean:
	$(RM) -r gcc

### GNU Debugger

gdb-configure: gdb/.configure
gdb/.configure: gcc/.install
	$(MKDIR) gdb && $(CD) gdb && PATH=$(DESTDIR)$(PREFIX)/bin:$$PATH $(SRCDIR)/gdb/configure \
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
	$(TOUCH) $@

gdb-build: gdb/.build
gdb/.build: gdb/.configure
	$(CD) gdb && $(MAKE) LDFLAGS=-Wl,--as-needed
	$(TOUCH) $@

gdb-install: gdb/.install
gdb/.install: gdb/.build
	$(CD) gdb && $(MAKE) install-gdb DESTDIR=$(DESTDIR)
	$(TOUCH) $@

gdb-clean:
	$(RM) -r gdb


.PHONY: $(foreach util, binutils gcc gdb, $(foreach op, configure build install clean, $(util)-$(op)))
