# vim: tabstop=8 shiftwidth=8 noexpandtab:

SUBDIR-install = $(SUBDIR:%=%-install)

$(SUBDIR):
	$(MAKE) -C $@

# install targets represent the installation of a program into sysroot.
%-install: %
	$(MAKE) -C $< install

clean:
	for dir in $(SUBDIR); do \
	  $(MAKE) -C $$dir clean; \
	done

.PHONY: $(SUBDIR)

include $(TOPDIR)/build/common.mk
