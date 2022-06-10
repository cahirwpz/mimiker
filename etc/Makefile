# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ..)

FILES = group master.passwd .kshrc shells
INSTALL-FILES = $(addprefix $(SYSROOT)/etc/, $(FILES))

all: build

include $(TOPDIR)/build/common.mk

$(SYSROOT)/etc/%: %
	echo "[INSTALL] $(DIR)$* -> /etc/$*"
	$(INSTALL) -m 644 -D $< $@
