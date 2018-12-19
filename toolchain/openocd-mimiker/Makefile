#!/usr/bin/make -f

VER := 0.10.0+dev
OPENOCD := openocd-$(VER)

all: binary

download:
	git clone https://git.code.sf.net/p/openocd/code $(OPENOCD)

configure:
	rm -rf openocd-build
	mkdir openocd-build
	cd $(OPENOCD) && \
	    ./bootstrap
	cd openocd-build && \
	    ../$(OPENOCD)/configure \
	        --prefix=/usr \
		--enable-jlink 
	touch $@-stamp

build: configure
	cd openocd-build && make -j$(shell nproc --all) LDFLAGS="-Wl,--as-needed"
	touch $@-stamp

install: build
	cd openocd-build && make install DESTDIR=$(PWD)/debian/tmp
	cd $(PWD)/debian/tmp && \
		mv usr/bin/openocd usr/bin/openocd-mimiker && \
		mv usr/share/openocd/ usr/share/openocd-mimiker
	touch $@-stamp

binary: install
	fakeroot ./debian/rules binary
	touch $@-stamp

clean: 
	dh_clean
	rm -rf $(OPENOCD) openocd-build
	rm -f *~

.PHONY: download install binary clean
