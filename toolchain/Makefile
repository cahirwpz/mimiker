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

build-aarch64:
	$(TOOLCHAIN) --target=aarch64 build
	cd aarch64-mimiker-elf && fakeroot ./debian/rules binary

clean-mipsel:
	$(TOOLCHAIN) --target=aarch64 clean
	cd mipsel-mimiker-elf && \
	  $(RM) -r sources usr

build: build-mipsel build-aarch64

clean: clean-mipsel clean-aarch64
	$(RM) *~

.PHONY: all build build-mipsel build-aarch64 clean clean-mipsel clean-aarch64

# vim: ts=8 sw=8 noet
