# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

HEADERS = $(wildcard *.h) $(foreach dir, machine sys, $(wildcard $(dir)/*.h))
INSTALL-FILES = $(addprefix $(SYSROOT)/usr/include/, $(HEADERS))

SOURCES_H = $(foreach dir, . dev aarch64 mips riscv sys, $(wildcard $(dir)/*.h))

all: build

include $(TOPDIR)/build/common.mk

setup: machine

machine: $(TOPDIR)/config.mk
	@echo "[SYMLINK] machine -> $(ARCH)"
	$(RM) machine
	$(LN) -s $(ARCH) machine

distclean-here:
	$(RM) -v machine

$(SYSROOT)/usr/include/%: %
	echo "[INSTALL] $(DIR)$* -> /usr/include/$*"
	$(INSTALL) -m 644 -D $< $@
