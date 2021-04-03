# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(CURDIR)

# Directories which require calling make recursively
ifeq ($(BOARD), pc)
# XXX: just build the kernel for now.
SUBDIR = sys include
else
SUBDIR = sys lib bin usr.bin etc include
endif

all: install

format: setup
build: setup
install: build
distclean: clean

bin-before: lib-install
usr.bin-before: lib-install

# Detecting whether initrd.cpio requires rebuilding is tricky, because even if
# this target was to depend on $(shell find sysroot -type f), then make compares
# sysroot files timestamps BEFORE recursively entering bin and installing user
# programs into sysroot. This sounds silly, but apparently make assumes no files
# appear "without their explicit target". Thus, the only thing we can do is
# forcing make to always rebuild the archive.
ifeq ($(BOARD), pc)
INITRD_DEPENDENCIES = sys-install
else
INITRD_DEPENDENCIES = bin-install
endif

initrd.cpio: $(INITRD_DEPENDENCIES)
	@echo "[INITRD] Building $@..."
	cd sysroot && \
	  find -depth \( ! -name "*.dbg" -and -print \) | sort | \
	    $(CPIO) -o -R +0:+0 -F ../$@ 2> /dev/null

DISK_SIZE = 1073741824  # 1GiB

disk.img:
	@echo "[DISK] Building $@..."
	dd if=/dev/zero of=disk.img bs=512 \
	   count=$(shell echo '$(DISK_SIZE) / 512' | bc)

# Creating the boot partition is platform dependent.
# TODO: create and populate partitions besides the boot partition.
disk: initrd.cpio disk.img
	$(MAKE) -C sys/$(ARCH) disk

INSTALL-FILES += initrd.cpio disk
CLEAN-FILES += initrd.cpio

distclean-here:
	$(RM) -r sysroot disk.img

setup:
	$(MAKE) -C include setup

test: sys-build initrd.cpio
	./run_tests.py --board $(BOARD)

PHONY-TARGETS += setup test disk

IMGVER = 1.8.0
IMGNAME = cahirwpz/mimiker-ci

docker-build:
	docker build . -t $(IMGNAME):latest

docker-tag:
	docker image tag $(IMGNAME):latest $(IMGNAME):$(IMGVER)

docker-push:
	docker push $(IMGNAME):latest
	docker push $(IMGNAME):$(IMGVER)

PHONY-TARGETS += docker-build docker-tag docker-push

include $(TOPDIR)/build/common.mk
