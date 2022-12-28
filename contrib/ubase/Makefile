TOPDIR = $(realpath ../..)

.SUFFIXES:
.SUFFIXES: .o .c

HDR =\
	arg.h\
	compat.h\
	crypt.h\
	fs.h\
	md5.h\
	queue.h\
	sha1.h\
	sha224.h\
	sha256.h\
	sha384.h\
	sha512.h\
	sha512-224.h\
	sha512-256.h\
	text.h\
	utf.h\
	util.h

LIBUTIL = libutil.a
LIBUTILSRC = \
	libutil/agetcwd.c        \
	libutil/agetline.c       \
	libutil/apathmax.c       \
	libutil/concat.c         \
	libutil/ealloc.c         \
	libutil/eprintf.c        \
	libutil/estrtol.c        \
	libutil/estrtoul.c       \
	libutil/explicit_bzero.c \
	libutil/proc.c           \
	libutil/putword.c        \
	libutil/recurse.c        \
	libutil/strlcat.c        \
	libutil/strlcpy.c        \
	libutil/strtonum.c       \
	libutil/tty.c

LIBUTILSRC_DONT_WORK = \
	libutil/passwd.c

LIB = $(LIBUTIL)

BIN = \
	clear             \
	id                \
	nologin           \
	pagesize          \
	stat              \
	truncate          \
	watch

BIN_DONT_WORK = \
	blkdiscard        \
	chvt              \
	ctrlaltdel        \
	dd                \
	df                \
	dmesg             \
	eject             \
	fallocate         \
	free              \
	freeramdisk       \
	fsfreeze          \
	getty             \
	halt              \
	hwclock           \
	insmod            \
	killall5          \
	last              \
	lastlog           \
	login             \
	lsmod             \
	lsusb             \
	mesg              \
	mknod             \
	mkswap            \
	mount             \
	mountpoint        \
	passwd            \
	pidof             \
	pivot_root        \
	ps                \
	pwdx              \
	readahead         \
	respawn           \
	rmmod             \
	su                \
	swaplabel         \
	swapoff           \
	swapon            \
	switch_root       \
	sysctl            \
	umount            \
	unshare           \
	uptime            \
	vtallow           \
	who

CFLAGS.ubase/clear.c += -Dmain=clear_main
CFLAGS.ubase/id.c += -Dmain=id_main
CFLAGS.ubase/nologin.c += -Dmain=nologin_main
CFLAGS.ubase/pagesize.c += -Dmain=pagesize_main
CFLAGS.ubase/stat.c += -Dmain=stat_main
CFLAGS.ubase/truncate.c += -Dmain=truncate_main
CFLAGS.ubase/watch.c += -Dmain=watch_main

SOURCES_UBASE = $(addprefix ubase/,$(BIN:=.c))
SOURCES_LIBUTIL = $(addprefix ubase/,$(LIBUTILSRC))
OBJ_LIBUTIL = $(SOURCES_LIBUTIL:%.c=$(BUILDDIR)%.o)
LIB_UBASE = $(addprefix ubase/,$(LIBUTIL))

PROGRAM = ubase-box
BINDIR = bin/ubase
BUILDDIR = build/
SOURCES = $(SOURCES_UBASE)
FORMAT-EXCLUDE = $(SOURCES_UBASE) $(SOURCES_LIBUTIL) $(addprefix ubase/,$(HDR))

OBJECTS += $(addprefix $(BUILDDIR),$(LIB_UBASE)) $(addsuffix .o,$(addprefix $(BUILDDIR),$(PROGRAM)))
DEPENDENCY-FILES += $(foreach f, $(SOURCES_LIBUTIL),\
		      $(BUILDDIR)$(dir $(f))$(patsubst %.c,%.d,$(notdir $(f))))
CFLAGS.ubase/stat.c += -Wno-error=format

include $(TOPDIR)/build/build.prog.mk

$(PROGRAM).c:
	@echo "[PYTHON] $(BIN) -> $@"
	./box_src_generator.py --program $(PROGRAM) --tools $(BIN)

$(BUILDDIR)ubase/$(LIBUTIL): $(OBJ_LIBUTIL)

download-here: ubase/.git

build-before: quilt-patch

install-here:
	@echo "[INSTALL] /bin/sbase/ubase-box"
	for f in $(BIN); do ln -sf $(PROGRAM) $(SYSROOT)/$(BINDIR)/"$$f"; done

clean-here: quilt-unpatch
	rm -f $(PROGRAM).c $(PROGRAM).uelf
