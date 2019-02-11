TOPDIR = $(realpath ../../)

SOURCEDIR = $(CURDIR)/newlib-3.0.0
BUILDDIR = $(CURDIR)/newlib-build

PKG = newlib-3.0.0
URL = ftp://sourceware.org/pub/newlib/$(PKG).tar.gz

all: download configure build install

download-here: $(PKG)

$(PKG).tar.gz:
	@echo "[CURL] $(PKG)"
	$(CURL) -# -O $(URL)

$(PKG): $(PKG).tar.gz
	@echo "[TAR] $^ -> $@"
	$(TAR) xzf $(PKG).tar.gz
	touch $(PKG)

# Output from these commands is redirected to log files for readibility.
configure: download configure-stamp
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

build-here: build-stamp
build-stamp: configure
	@echo "Compiling newlib..."
	@echo "(messages saved to 'newlib-build.log')"
	cd $(BUILDDIR) && $(MAKE) 2>&1 >newlib-build.log
	touch $@

install-here: install-stamp
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

clean-here:
	$(RM) install-stamp newlib-install.log

distclean-here:
	$(RM) -r $(BUILDDIR) $(PKG) $(PKG).tar.gz
	$(RM) newlib-build.log configure-stamp build-stamp

PHONY-TARGETS += download configure

include $(TOPDIR)/build/tools.mk
include $(TOPDIR)/build/common.mk
