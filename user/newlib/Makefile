# That won't be necessary in the future, it's just for temporary convnience.
include ../../Makefile.common

# Newlib config
NEWLIB = newlib-2.5.0
NEWLIBFILE = $(NEWLIB).tar.gz
NEWLIBDIR = $(CACHEDIR)/$(NEWLIBDIR)
NEWLIB_DOWNLOADS = ftp://sourceware.org/pub/newlib

all: $(CACHEDIR)/$(NEWLIB)

install:
	cd $(CACHEDIR)/$(NEWLIB)/newlib; $(MAKE) install > /dev/null 2>&1

# The .tar.gz archive must be an order-only prerequisite, because the archive
# has a more recent last-modified date than the source directory.
$(CACHEDIR)/$(NEWLIB): | $(CACHEDIR)/$(NEWLIBFILE)
	@echo "Unpacking newlib..."
	tar -xzf $(CACHEDIR)/$(NEWLIBFILE) -C $(CACHEDIR)
	# Output from these commands is redirected to /dev/null for readibility. If
	# any of these commands fail, just remove output redirection.
	@echo "Configuring newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib; \
		CC=$(TRIPLET)-gcc ./configure --build=$(TRIPLET) --prefix=$(INSTALL_DIR)/usr > /dev/null
	@echo "Compiling newlib..."
	cd $(CACHEDIR)/$(NEWLIB)/newlib; $(MAKE) > /dev/null 2>&1

$(CACHEDIR)/$(NEWLIBFILE):
	@echo "Downloading $(NEWLIBFILE). Don't worry, the download is cached so" \
		"there will be no need to do this again."
	mkdir -p $(CACHEDIR)
	wget $(NEWLIB_DOWNLOADS)/$(NEWLIBFILE) -O $@ || \
		(ret=$$?; rm -rf $@; exit $$ret) # Remove incomplete newlib archive on a failed download

clean:
	true
