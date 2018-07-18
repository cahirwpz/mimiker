TOPDIR = $(realpath ../../)

include $(TOPDIR)/build/toolchain.mk
include $(TOPDIR)/build/tools.mk
include $(TOPDIR)/build/common.mk

SOURCEDIR = $(CURDIR)/newlib-3.0.0
BUILDDIR = $(CURDIR)/newlib-build

all: install

$(SOURCEDIR)/configure:
	@echo "Updating Git submodule..."
	git submodule update --init $(SOURCEDIR)

download: $(SOURCEDIR)/configure

# Output from these commands is redirected to log files for readibility.
configure: configure-stamp
configure-stamp: $(SOURCEDIR)/configure
	@echo "Configuring newlib..."
	@echo "(messages saved to 'newlib-config.log')"
	mkdir -p $(BUILDDIR) && cd $(BUILDDIR) && \
		CC="$(TARGET)-gcc -g" \
		AS="$(TARGET)-as -g" \
		AR="$(TARGET)-ar" \
		RANLIB="$(TARGET)-ranlib" \
		READELF="$(TARGET)-readelf" \
		CFLAGS="-Og" \
		CPPFLAGS="-DSIGNAL_PROVIDED" \
		$(SOURCEDIR)/newlib/configure \
		            --build="$(TARGET)" \
			    --prefix="$(SYSROOT)/usr" 2>&1 >newlib-config.log
	touch $@

build: build-stamp
build-stamp: configure-stamp
	@echo "Compiling newlib..."
	@echo "(messages saved to 'newlib-build.log')"
	cd $(BUILDDIR) && $(MAKE) 2>&1 >newlib-build.log
	touch $@

install: install-stamp
install-stamp: build-stamp
	@echo "Installing newlib..."
	@echo "(messages saved to 'newlib-install.log')"
	cd $(BUILDDIR) && $(MAKE) install 2>&1 >newlib-install.log
	# Copy overrides. TODO: These files should be copied into $(NEWLIB)
	# before it is compiled, so that all references within newlib sources
	# are using our version of the override file. But for now it does not
	# matter, and it's simpler to copy them directly to $(SYSROOT).
	cp -RL overrides/* $(SYSROOT)/usr/
	touch $@

clean:
	$(RM) install-stamp newlib-install.log
	$(RM) *~

distclean: clean
	$(RM) -r $(BUILDDIR)
	$(RM) *-stamp newlib-*.log

.PHONY:	download unpack configure build install distclean download
