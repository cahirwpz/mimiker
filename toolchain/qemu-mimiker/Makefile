#!/usr/bin/make -f

VER := 6.1.0
QEMU := qemu-$(VER)
ARCHIVE := $(QEMU).tar.xz

all: binary

download: $(ARCHIVE)
$(ARCHIVE):
	curl --progress-bar -O https://download.qemu.org/$(ARCHIVE)

unpack: unpack-stamp
unpack-stamp: $(ARCHIVE)
	tar xvf $(ARCHIVE)
	touch $@

patch: patch-stamp
patch-stamp: unpack-stamp
	quilt push -a
	touch $@

unpatch:
	if [ -f patch-stamp ]; then	\
		quilt pop -a;		\
		rm -f patch-stamp;	\
	fi

configure: configure-stamp
configure-stamp: patch-stamp
	rm -rf qemu-build
	mkdir qemu-build
	cd qemu-build && \
	    ../$(QEMU)/configure \
	        --prefix=/usr \
		--enable-fdt \
		--disable-docs \
		--disable-tools \
		--disable-kvm \
		--disable-guest-agent \
		--disable-vnc \
		--disable-libusb \
		--disable-curses \
		--disable-bzip2 \
		--disable-tpm \
		--disable-linux-aio \
	    	--target-list=mipsel-softmmu,aarch64-softmmu \
		--with-suffix=qemu-mimiker
	touch $@

build: build-stamp
build-stamp: configure-stamp
	cd qemu-build && make -j$(shell nproc --all) LDFLAGS="-Wl,--as-needed"
	touch $@

install: install-stamp
install-stamp: build-stamp
	cd qemu-build && make install DESTDIR=$(PWD)/debian/tmp
	cd $(PWD)/debian/tmp && \
	mv usr/bin/qemu-system-mipsel usr/bin/qemu-mimiker-mipsel && \
	mv usr/bin/qemu-system-aarch64 usr/bin/qemu-mimiker-aarch64 && \
	touch $@

binary: binary-stamp
binary-stamp: install-stamp
	fakeroot ./debian/rules binary
	touch $@

clean: unpatch
	dh_clean
	rm -rf $(QEMU) qemu-build
	rm -f *~

.PHONY: download unpack patch unpatch configure build install binary clean
