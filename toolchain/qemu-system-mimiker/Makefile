#!/usr/bin/make -f

VER := 2.12.0
QEMU := qemu-$(VER)
ARCHIVE := $(QEMU).tar.xz

all: binary

download: download-stamp
download-stamp:
	curl --progress-bar -O https://download.qemu.org/$(ARCHIVE)
	touch $@

unpack: unpack-stamp
unpack-stamp: download-stamp
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
		--disable-fdt \
		--disable-vnc \
		--disable-libusb \
		--disable-curses \
		--disable-bzip2 \
		--disable-linux-aio \
	    	--target-list=mimiker-softmmu
	touch $@

build: build-stamp
build-stamp: configure-stamp
	cd qemu-build && make -j$(shell nproc --all)
	touch $@

install: install-stamp
install-stamp: build-stamp
	cd qemu-build && make install DESTDIR=$(PWD)/debian/tmp
	touch $@

binary: binary-stamp
binary-stamp: install-stamp
	fakeroot ./debian/rules binary
	touch $@

clean: unpatch
	dh_clean
	rm -rf $(QEMU) qemu-build
	rm -f $(ARCHIVE)
	rm -f *-stamp *~

.PHONY: download unpack patch unpatch configure build install binary clean
