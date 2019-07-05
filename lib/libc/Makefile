# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

CFLAGS   = -ffreestanding
CPPFLAGS = -I$(TOPDIR)/include \
	   -I$(CURDIR)/include \
	   -I$(CURDIR)/citrus \
	   -I$(CURDIR)/locale \
	   -I$(CURDIR)/gen \
	   -D_LIBC

SOURCES = \
	gettimeofday.c \
	nanosleep.c \
	signal.c \
	syscalls.S \
	$(wildcard dummy/*.c) \
	$(wildcard gen/*.c) \
	$(wildcard locale/*.c) \
	$(wildcard stdio/*.c) \
	$(wildcard stdlib/*.c) \
	$(wildcard string/*.c) \
	$(wildcard time/*.c)


HEADERFILES = $(shell find $(TOPDIR)/include/ \! -iname '*~' \! -type d)
HEADERS = $(HEADERFILES:$(TOPDIR)/include/%=%)

LIBFILES = libc.a crt0.o ld.script

build: $(LIBFILES)

include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

$(LIBFILES): $(OBJECTS)

EXTRAFILES = $(shell find extra -type f)

# Installation targets
LIBFILES_INST = $(LIBFILES:%=$(SYSROOT)/lib/%)
EXTRAFILES_INST = $(EXTRAFILES:extra/%=$(SYSROOT)/%)
HEADERS_INST = $(HEADERS:%=$(SYSROOT)/usr/include/%)

install-here: $(LIBFILES_INST) $(EXTRAFILES_INST) $(HEADERS_INST)

$(LIBFILES_INST): $(LIBFILES)
	for LIBFILE in $(LIBFILES); do \
	  echo "[INSTALL] $(DIR)$$LIBFILE -> /lib/$$LIBFILE"; \
	  $(INSTALL) -m 644 $$LIBFILE $(SYSROOT)/lib/$$LIBFILE; \
	done

$(HEADERS_INST): $(HEADERFILES)
	mkdir -p $(SYSROOT)/usr/include
	for HEADER in $(HEADERS); do \
	  echo "[INSTALL] $(DIR)$$HEADER -> /usr/include/$$HEADER"; \
	  src=$(TOPDIR)/include/$$HEADER; \
	  dst=$(SYSROOT)/usr/include/$$HEADER; \
	  if [ -f $$src ]; then \
	    $(INSTALL) -m 644 $$src $$dst; \
	  else \
	    $(CP) -a $$src $$dst; \
	  fi; \
	done

$(SYSROOT)/%: extra/%
	@echo "[INSTALL] $(DIR)$< -> $*"
	$(INSTALL) -m 644 $< $@

clean-here:
	$(RM) *.o *.a
