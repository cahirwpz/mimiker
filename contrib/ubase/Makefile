TOPDIR = $(realpath ../..)


HDR = \
	arg.h \
	compat.h \
	crypt.h \
	fs.h \
	md5.h \
	queue.h \
	sha1.h \
	sha224.h \
	sha256.h \
	sha384.h \
	sha512.h \
	sha512-224.h \
	sha512-256.h \
	text.h \
	utf.h \
	util.h

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
	libutil/passwd.c         \
	libutil/proc.c           \
	libutil/putword.c        \
	libutil/recurse.c        \
	libutil/strlcat.c        \
	libutil/strlcpy.c        \
	libutil/strtonum.c       \
	libutil/tty.c

BIN = \
	clear             \
	getty             \
	id                \
	login             \
	mesg              \
	nologin           \
	pagesize          \
	respawn           \
	stat              \
	su                \
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
	halt              \
	hwclock           \
	insmod            \
	killall5          \
	last              \
	lastlog           \
	lsmod             \
	lsusb             \
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
	rmmod             \
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

define rename_main_func =
CFLAGS.ubase/$(1).c += -Dmain=$$(subst -,_,$(1))_main
endef

$(foreach bin,$(BIN),$(eval $(call rename_main_func,$(bin))))

PROGRAM = ubase-box
BINDIR = bin
BUILDDIR = build/
SOURCES = $(PROGRAM).c $(addprefix ubase/,$(BIN:=.c)) $(addprefix ubase/,$(LIBUTILSRC))
FORMAT-EXCLUDE = $(SOURCES) $(addprefix ubase/,$(HDR))

CFLAGS.ubase/stat.c += -Wno-error=format

include $(TOPDIR)/build/build.prog.mk

$(PROGRAM).c:
	@echo "[PYTHON] generating $@"
	./box_src_generator.py --program $(PROGRAM) --tools $(BIN)

download-here: ubase/.git

build-before: quilt-patch

install-here:
	@echo "[INSTALL] $(BINDIR)/$(PROGRAM)"
	for f in $(BIN); do ln -sf $(PROGRAM) $(SYSROOT)/$(BINDIR)/"$$f"; done

clean-here: quilt-unpatch
	rm -f $(PROGRAM).c $(PROGRAM).uelf

$(OBJECTS): $(BUILDDIR)ubase/config.h

$(BUILDDIR)%.h: %.def.h
	cp $< $@

CPPFLAGS += -I$(BUILDDIR)/ubase
