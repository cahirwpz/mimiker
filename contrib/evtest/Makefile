TOPDIR = $(realpath ../..)

EVTEST_URL = https://github.com/freedesktop-unofficial-mirror/evtest.git

PROGRAM = evtest
BINDIR = usr/bin
SOURCES = evtest/evtest.c versionsort.c
PATCHES = 01.patch
CLEAN-FILES = patch-stamp

FORMAT-EXCLUDE = $(SOURCES)

include $(TOPDIR)/build/build.prog.mk

all: download build install

download-here: evtest patch

evtest:
	@echo "[GIT] clone $(EVTEST_URL) -> $@"
	$(GIT) clone $(EVTEST_URL) $@
	make -C .

patch: patch-stamp
patch-stamp:
	@for p in $(PATCHES); do \
	  echo "[PATCH] apply $$p"; \
	  $(PATCH) -d evtest -p1 < $$p; \
	done
	touch $@

unpatch:
	@if [ -f patch-stamp ]; then \
	  for p in $(PATCHES); do \
	    echo "[PATCH] revert $$p"; \
	    $(PATCH) -d evtest -R -p1 < $$p; \
	  done; \
	fi
	$(RM) patch-stamp

clean-here: unpatch

distclean-here:
	$(RM) -r evtest
	$(RM) patch-stamp

WFLAGS += -Wno-strict-prototypes -Wno-implicit-fallthrough -Wno-sign-compare \
          -Wno-char-subscripts -Wno-override-init
