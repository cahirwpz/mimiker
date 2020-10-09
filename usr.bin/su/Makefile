TOPDIR = $(realpath ../..)

PROGRAM = su
SOURCES = su.c suutil.c
FORMAT-EXCLUDE = $(SOURCES)

LDLIBS = -lutil


CPPFLAGS += -DALLOW_EMPTY_USER# -DALLOW_GROUP_CHANGE for future use

include $(TOPDIR)/build/build.prog.mk
