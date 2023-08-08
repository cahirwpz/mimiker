TOPDIR = $(realpath ../..)

PROGRAM = st
SOURCES = win.c font.c input.c st.c

LDLIBS = -lutil

# TODO: Why those are needed with gcc on MIPS ?
WFLAGS = -Wno-error=strict-prototypes -Wno-error=unused-variable \
	 -Wno-error=type-limits -Wno-error=implicit-fallthrough \
	 -Wno-error=maybe-uninitialized

include $(TOPDIR)/build/build.prog.mk
