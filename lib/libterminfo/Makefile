# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = hash.c compile.c term.c termcap.c ti.c setupterm.c \
	curterm.c tparm.c tputs.c

include $(TOPDIR)/build/build.lib.mk

CPPFLAGS += -DTERMINFO_COMPILE -DTERMINFO_DB -DTERMINFO_COMPAT
WFLAGS += -Wno-implicit-fallthrough -Wno-type-limits -Wno-unused-const-variable

# Generate our string and hash tables
hash.c: genhash $(TOPDIR)/include/term.h
		@echo "Generating terminfo hash"
		sh $^ > $@

termcap_hash.c: genthash termcap_map.c
		@echo "Generating termcap hash"
		sh $^ > $@

CLEAN-FILES += termcap_hash.c hash.c

