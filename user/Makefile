# vim: tabstop=8 shiftwidth=8 noexpandtab:

include ../Makefile.common

THISDIR = $(realpath $(dir $(filter %Makefile, $(MAKEFILE_LIST))))
PROGDIRS = $(shell find -mindepth 1 -maxdepth 1 -type d -not -name "sysroot*" -and -not -name "libmimiker" -and -not -name "cache")
CACHEDIR = $(TOPDIR)/cache
NEWLIB = newlib-2.5.0
NEWLIBFILE = $(NEWLIB).tar.gz
NEWLIBDIR = $(CACHEDIR)/$(NEWLIBDIR)
NEWLIB_DOWNLOADS = ftp://sourceware.org/pub/newlib

.PHONY: clean all $(PROGDIRS) libmimiker

all: $(PROGDIRS) | sysroot

$(PROGDIRS): sysroot
	$(MAKE) -C $@

sysroot: sysroot-base sysroot-newlib libmimiker/libmimiker.a
	@echo "Rebuilding sysroot..."
	rm -rf sysroot && mkdir sysroot
	# Install newlib into sysroot
	cp -r sysroot-newlib/ sysroot/usr
	# Instal libmimiker into sysroot
	cp libmimiker/libmimiker.a sysroot/usr/lib/.
	# Instal custom files into sysroot
	cp -rT sysroot-base/ sysroot/

libmimiker/libmimiker.a: libmimiker
	$(MAKE) -C libmimiker libmimiker.a

sysroot-newlib: $(CACHEDIR)/$(NEWLIB)
	# Output from these commands is redirected to /dev/null for readibility. If
	# any of these commands fail, just remove output redirection.
	rm -rf sysroot-newlib && mkdir -p sysroot-newlib
	@echo "Configuring newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib; \
		CC=$(TRIPLET)-gcc ./configure --build=$(TRIPLET) --prefix=$(THISDIR)/$@ > /dev/null
	@echo "Compiling newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib; $(MAKE) > /dev/null 2>&1
	@echo "Installing newlib to $(DIR)/$@..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib; $(MAKE) install > /dev/null 2>&1

# The .tar.gz archive must be an order-only prerequisite, because the archive
# has a more recent last-modified date than the source directory.
$(CACHEDIR)/$(NEWLIB): | $(CACHEDIR)/$(NEWLIBFILE)
	@echo "Unpacking newlib..."
	tar -xzf $(CACHEDIR)/$(NEWLIBFILE) -C $(CACHEDIR)

$(CACHEDIR)/$(NEWLIBFILE):
	@echo "Downloading $(NEWLIBFILE). Don't worry, the download is cached so" \
		"there will be no need to do this again."
	mkdir -p $(CACHEDIR)
	wget $(NEWLIB_DOWNLOADS)/$(NEWLIBFILE) -O $@ || \
		(ret=$$?; rm -rf $@; exit $$ret) # Remove incomplete newlib archive on a failed download

clean:
	rm -rf sysroot
	for dir in $(PROGDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done
	$(MAKE) -C libmimiker clean
	# Don't remove sysroot-newlib, where compiled & installed newlib files are
	# stored. Until we introduce major modifications to newlib, keeping that
	# directory can't cause any harm, and improves build times.  At some point
	# we may want to introduce a `distclean` target which would also remove
	# cache and sysroot-newlib.
