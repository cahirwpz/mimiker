# vim: tabstop=8 shiftwidth=8 noexpandtab:

$(SUBDIR):
	$(MAKE) -C $@

# install targets represent the installation of a program into sysroot.
install:
	for dir in $(SUBDIR); do \
	  $(MAKE) -C $$dir install; \
	done

clean:
	for dir in $(SUBDIR); do \
	  $(MAKE) -C $$dir clean; \
	done

.PHONY: $(SUBDIR)

include $(TOPDIR)/build/common.mk
