# vim: tabstop=8 shiftwidth=8 noexpandtab:

TOPDIR = $(realpath ../..)

SOURCES = hash.c compile.c term.c termcap.c ti.c setupterm.c \
	curterm.c tparm.c tputs.c
CPPFLAGS += -DTERMINFO_COMPILE -DTERMINFO_DB -DTERMINFO_COMPAT
CFLAGS += -Wno-implicit-fallthrough -Wno-type-limits

include $(TOPDIR)/build/build.lib.mk

# Generate our string and hash tables
hash.c: genhash $(TOPDIR)/include/term.h
		@echo "Generating terminfo hash"
		CC="$(CC)" sh $^ > $@

termcap_hash.c: genthash termcap_map.c
		@echo "Generating termcap hash"
		CC="$(CC)" sh $^ > $@

CLEAN-FILES += termcap_hash.c hash.c

