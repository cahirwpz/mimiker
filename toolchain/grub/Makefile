# vim: tabstop=8 shiftwidth=8 noexpandtab list:

VERSION := 2.04
GRUB := grub-$(VERSION)
ARCHIVE := $(GRUB).tar.xz

TOPDIR := $(CURDIR)

all: package

download: $(ARCHIVE)
$(ARCHIVE):
	curl -o $@ https://ftp.gnu.org/gnu/grub/$(ARCHIVE)

unpack: unpack-stamp
unpack-stamp: $(ARCHIVE)
	tar xvf $^
	touch $@

configure: configure-stamp
configure-stamp: unpack-stamp
	rm -rf grub-build
	mkdir grub-build
	cd grub-build && \
		../$(GRUB)/configure \
			--prefix=/usr \
			--target=x86_64 \
			--with-platform=efi
	touch $@

build: build-stamp
build-stamp: configure-stamp
	cd grub-build $$ make -j$(shell nproc --all)
	touch $@

install: install-stamp
install-stamp: build-stamp
	cd grub-build && make install DESTDIR=$(PWD)/debian/tmp
	touch $@

package: package-stamp
package-stamp: install-stamp
	fakeroot $(PWD)/debian/rules binary
	touch $@

clean:
	dh_clean
	rm -rf $(GRUB) grub-build
	rm -f *~

distclean: clean
	rm -f $(ARCHIVE)

.PHONY: all download unpack configure build install package clean distclean
