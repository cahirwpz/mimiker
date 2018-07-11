TOPDIR = $(realpath ../../)

include $(TOPDIR)/build/toolchain.mk
include $(TOPDIR)/build/tools.mk
include $(TOPDIR)/build/common.mk

SOURCEDIR = $(CURDIR)/newlib-2.5.0
BUILDDIR = $(CURDIR)/newlib-build

all: install

$(SOURCEDIR)/configure:
	@echo "Updating Git submodule..."
	git submodule update --init $(SOURCEDIR)

download: $(SOURCEDIR)/configure

# Output from these commands is redirected to /dev/null for readibility.
# If any of these commands fail, just remove output redirection.
configure: configure-stamp
configure-stamp: $(SOURCEDIR)/configure
	@echo "Configuring newlib..."
	mkdir -p $(BUILDDIR) && cd $(BUILDDIR) && \
		CC="$(TARGET)-gcc" \
		AS="$(TARGET)-as" \
		AR="$(TARGET)-ar" \
		RANLIB="$(TARGET)-ranlib" \
		READELF="$(TARGET)-readelf" \
		$(SOURCEDIR)/newlib/configure \
		            --build="$(TARGET)" \
		            --prefix="$(SYSROOT)/usr" \
		            CFLAGS="-g -DSIGNAL_PROVIDED" 2>&1 >/dev/null
	touch $@

build: build-stamp
build-stamp: configure-stamp
	@echo "Compiling newlib..."
	$(MAKE) -C $(BUILDDIR) 2>/dev/null && touch $@

install: install-stamp
install-stamp: build-stamp
	@echo "Installing newlib..."
	$(MAKE) -C $(BUILDDIR) install 2>&1 >/dev/null
	# Copy overrides. TODO: These files should be copied into $(NEWLIB)
	# before it is compiled, so that all references within newlib sources
	# are using our version of the override file. But for now it does not
	# matter, and it's simpler to copy them directly to $(SYSROOT).
	cp -RL overrides/* $(SYSROOT)/usr/
	touch $@

clean:
	$(RM) install-stamp

distclean:
	$(RM) -r $(BUILDDIR)
	$(RM) *-stamp

.PHONY:	download unpack configure build install distclean download
