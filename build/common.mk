SYSROOT  = $(TOPDIR)/sysroot
CACHEDIR = $(TOPDIR)/cache
DIR = $(patsubst $(TOPDIR)/%,%,$(CURDIR)/)

# Pass "VERBOSE=1" at command line to display command being invoked by GNU Make
ifneq ($(VERBOSE), 1)
.SILENT:
endif

.PHONY: all clean install
