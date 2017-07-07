TOPDIR = $(realpath ../../)

include $(TOPDIR)/build/toolchain.mk
include $(TOPDIR)/build/tools.mk
include $(TOPDIR)/build/common.mk

# Newlib config
NEWLIB = newlib-2.5.0
NEWLIBFILE = $(NEWLIB).tar.gz
NEWLIBDIR = $(CACHEDIR)/$(NEWLIBDIR)
NEWLIBURL = ftp://sourceware.org/pub/newlib/$(NEWLIBFILE)

all: install

download: download-stamp
download-stamp:
	@echo "Downloading $(NEWLIBFILE)..."
	@echo "Don't worry, the download is cached so there will be no need " \
	      "to do this again."
	mkdir -p $(CACHEDIR)
	# Remove incomplete newlib archive on a failed download 
	wget $(NEWLIBURL) -O $(CACHEDIR)/$(NEWLIBFILE) || \
		(ret=$$?; rm -rf $(CACHEDIR)/$(NEWLIBFILE); exit $$ret)
	touch $@

# The .tar.gz archive must be an order-only prerequisite, because the archive
# has a more recent last-modified date than the source directory.
unpack: unpack-stamp
unpack-stamp: download-stamp
	@echo "Unpacking newlib..."
	cd $(CACHEDIR) && tar -xzf $(NEWLIBFILE)
	touch $@

# Output from these commands is redirected to /dev/null for readibility.
# If any of these commands fail, just remove output redirection.
configure: configure-stamp
configure-stamp: unpack-stamp
	@echo "Configuring newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib && \
		CC="$(TARGET)-gcc" \
		AS="$(TARGET)-as" \
		AR="$(TARGET)-ar" \
		RANLIB="$(TARGET)-ranlib" \
		READELF="$(TARGET)-readelf" \
		./configure --build=$(TARGET) \
		            --prefix=$(SYSROOT)/usr \
		            CFLAGS="-g -DSIGNAL_PROVIDED" > /dev/null
	touch $@

build: build-stamp
build-stamp: configure-stamp
	@echo "Compiling newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib && $(MAKE) -s 2>&1
	touch $@

install: install-stamp
install-stamp: build-stamp
	@echo "Installing newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib && $(MAKE) install >/dev/null 2>&1
	# Copy overrides. TODO: These files should be copied into $(NEWLIB)
	# before it is compiled, so that all references within newlib sources
	# are using our version of the override file. But for now it does not
	# matter, and it's simpler to copy them directly to $(SYSROOT).
	cp -RL overrides/* $(SYSROOT)/usr/
	touch $@

clean:
	$(RM) install-stamp

distclean:
	$(rm) *-stamp

.PHONY:	download unpack configure build install
