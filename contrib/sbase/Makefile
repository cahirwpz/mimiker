TOPDIR = $(realpath ../..)

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

LIBUTFSRC =\
	libutf/fgetrune.c\
	libutf/fputrune.c\
	libutf/isalnumrune.c\
	libutf/isalpharune.c\
	libutf/isblankrune.c\
	libutf/iscntrlrune.c\
	libutf/isdigitrune.c\
	libutf/isgraphrune.c\
	libutf/isprintrune.c\
	libutf/ispunctrune.c\
	libutf/isspacerune.c\
	libutf/istitlerune.c\
	libutf/isxdigitrune.c\
	libutf/lowerrune.c\
	libutf/rune.c\
	libutf/runetype.c\
	libutf/upperrune.c\
	libutf/utf.c\
	libutf/utftorunestr.c

LIBUTILSRC =\
	libutil/concat.c\
	libutil/cp.c\
	libutil/crypt.c\
	libutil/ealloc.c\
	libutil/enmasse.c\
	libutil/eprintf.c\
	libutil/eregcomp.c\
	libutil/estrtod.c\
	libutil/fnck.c\
	libutil/fshut.c\
	libutil/getlines.c\
	libutil/human.c\
	libutil/linecmp.c\
	libutil/md5.c\
	libutil/memmem.c\
	libutil/mkdirp.c\
	libutil/mode.c\
	libutil/parseoffset.c\
	libutil/putword.c\
	libutil/reallocarray.c\
	libutil/recurse.c\
	libutil/rm.c\
	libutil/sha1.c\
	libutil/sha224.c\
	libutil/sha256.c\
	libutil/sha384.c\
	libutil/sha512.c\
	libutil/sha512-224.c\
	libutil/sha512-256.c\
	libutil/strcasestr.c\
	libutil/strlcat.c\
	libutil/strlcpy.c\
	libutil/strsep.c\
	libutil/strtonum.c\
	libutil/unescape.c\
	libutil/writeall.c

BIN =\
	basename\
	cat\
	chgrp\
	chmod\
	chown\
	cmp\
	comm\
	cp\
	cut\
	dd\
	dirname\
	echo\
	env\
	expand\
	expr\
	false\
	fold\
	head\
	link\
	ln\
	logname\
	md5sum\
	mkdir\
	mkfifo\
	mktemp\
	mv\
	nice\
	nl\
	nohup\
	printenv\
	pwd\
	readlink\
	renice\
	rev\
	rm\
	rmdir\
	seq\
	setsid\
	sha1sum\
	sha224sum\
	sha256sum\
	sha384sum\
	sha512sum\
	sha512-224sum\
	sha512-256sum\
	sleep\
	split\
	sponge\
	strings\
	sync\
	test\
	tr\
	true\
	tsort\
	tty\
	unexpand\
	uniq\
	unlink\
	uudecode\
	uuencode\
	wc\
	which\
	whoami\
	xinstall\
	yes\
	cal\
	cols\
	cksum\
	find\
	grep\
	join\
	kill\
	ls\
	od\
	paste\
	pathchk\
	printf\
	sed\
	sort\
	tail\
	tar\
	time\
	tee\
	xargs

BIN_DONT_WORK =\
	date\
	getconf\
	touch\

BIN_QUESTIONABLE =\
	cron\
	du\
	ed\
	logger

BIN_SURELY_DISABLED =\
	chroot\
	flock\
	hostname\
	mknod\
	tftp

define rename_main_func =
CFLAGS.sbase/$(1).c += -Dmain=$$(subst -,_,$(1))_main
endef

$(foreach bin,$(BIN),$(eval $(call rename_main_func,$(bin))))

PROGRAM = sbase-box
BINDIR = bin/sbase
BUILDDIR = build/
SOURCES = $(addprefix sbase/,$(BIN:=.c)) $(addprefix sbase/,$(LIBUTILSRC)) $(addprefix sbase/,$(LIBUTFSRC))
FORMAT-EXCLUDE = $(SOURCES) $(addprefix sbase/,$(HDR))

OBJECTS += $(addsuffix .o,$(addprefix $(BUILDDIR),$(PROGRAM)))
WFLAGS += -Wno-error=sign-compare -Wno-error=implicit-fallthrough
CFLAGS.sbase/join.c += -Wno-error=sometimes-uninitialized
CFLAGS.sbase/find.c += -Wno-error=tautological-constant-out-of-range-compare

include $(TOPDIR)/build/build.prog.mk

$(PROGRAM).c:
	@echo "[PYTHON] generating $@"
	./box_src_generator.py --program $(PROGRAM) --tools $(BIN)

download-here: sbase/.git

build-before: quilt-patch

install-here:
	@echo "[INSTALL] $(BINDIR)/$(PROGRAM)"
	for f in $(BIN); do ln -sf $(PROGRAM) $(SYSROOT)/$(BINDIR)/"$$f"; done
	ln -sf $(PROGRAM)  $(SYSROOT)/$(BINDIR)/[

clean-here: quilt-unpatch
	rm -f $(PROGRAM).c $(PROGRAM).uelf