# vim: tabstop=8 shiftwidth=8 noexpandtab:

include ../../Makefile.common

LIBFILE = libmimiker.a
EXTRAFILES = $(shell find extra -type f)

# Installation targets
LIBFILE_INST = $(INSTALL_DIR)/usr/lib/$(LIBFILE)
EXTRAFILES_INST = $(EXTRAFILES:extra/%=$(INSTALL_DIR)/%)

all: $(LIBFILE)
install: $(INSTALL_DIR)/usr/lib/$(LIBFILE) $(EXTRAFILES_INST)

$(LIBFILE_INST): $(LIBFILE)
	@echo "[INSTALL] $(DIR)$(LIBFILE) -> /usr/lib/$(LIBFILE)"
	install -m 755 -D $(LIBFILE) $(LIBFILE_INST)
$(INSTALL_DIR)/%: extra/%
	@echo "[INSTALL] $(DIR)$< -> $*"
	install -m 755 -D $< $@

libmimiker.a: start.o syscalls.o

clean:
	rm -f *.o *.a
