TOOLCHAIN = ./toolchain
RM = rm -v -f

all: build

build-mipsel:
	$(TOOLCHAIN) --target=mipsel build
	cd mipsel-mimiker-elf && fakeroot ./debian/rules binary

clean-mipsel:
	$(TOOLCHAIN) --target=mipsel clean
	$(RM) mipsel-*.deb
	cd mipsel-mimiker-elf && \
	  $(RM) -r sources usr debian/mipsel-mimiker-elf.*

build: build-mipsel

clean: clean-mipsel
	$(RM) *~

.PHONY: all build build-mipsel clean clean-mipsel

# vim: ts=8 sw=8 noet
