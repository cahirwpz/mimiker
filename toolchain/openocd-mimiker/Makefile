#!/usr/bin/make -f

VER := 0.10.0+dev
OPENOCD := openocd-$(VER)

all: binary

download: $(OPENOCD)
$(OPENOCD) :
	git clone https://git.code.sf.net/p/openocd/code $(OPENOCD)

configure: configure-stamp
configure-stamp: $(OPENOCD)
	rm -rf openocd-build
	mkdir openocd-build
	cd $(OPENOCD) && \
	    ./bootstrap && \
	sed -i 's/'"'"'openocd'"'"'/'"'"'openocd-mimiker'"'"'/g' configure
	cd openocd-build && \
	    ../$(OPENOCD)/configure \
		--prefix=/usr \
		--program-suffix='-mimiker' \
		--enable-jlink
	touch $@

build: build-stamp
build-stamp: configure-stamp
	cd openocd-build && make -j$(shell nproc --all) LDFLAGS="-Wl,--as-needed"
	touch $@

install: install-stamp
install-stamp: build-stamp
	cd openocd-build && make install DESTDIR=$(PWD)/debian/tmp
	touch $@

binary: binary-stamp
binary-stamp: install-stamp
	fakeroot ./debian/rules binary
	touch $@

clean:
	dh_clean
	rm -rf openocd-build
	rm -f *~

distclean: clean
	rm -rf $(OPENOCD)

.PHONY: download install binary clean
