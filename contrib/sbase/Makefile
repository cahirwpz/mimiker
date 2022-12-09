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

LIBUTF = libutf.a
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

LIBUTIL = libutil.a
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
	grep\
	ls\
	od\
	paste\
	printf\
	sed\
	sort\
	tee

BIN_DONT_WORK =\
	date\
	find\
	getconf\
	join\
	kill\
	pathchk\
	tail\
	tar\
	time\
	touch\
	xargs

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

SOURCES_SBASE = $(addprefix sbase/,$(BIN:=.c))
SOURCES_LIBUTIL = $(addprefix sbase/,$(LIBUTILSRC))
SOURCES_LIBUTF = $(addprefix sbase/,$(LIBUTFSRC))
OBJ_LIBUTIL = $(SOURCES_LIBUTIL:%.c=$(BUILDDIR)%.o)
OBJ_LIBUTF = $(SOURCES_LIBUTF:%.c=$(BUILDDIR)%.o)
LIB_SBASE = $(addprefix sbase/,$(LIBUTF)) $(addprefix sbase/,$(LIBUTIL))

PROGRAM = sbase-box
BINDIR = bin/sbase
BUILDDIR = build/
SOURCES = $(SOURCES_SBASE)
FORMAT-EXCLUDE = $(SOURCES_SBASE) $(SOURCES_LIBUTIL) $(SOURCES_LIBUTF) $(addprefix sbase/,$(HDR))

OBJECTS += $(addprefix $(BUILDDIR),$(LIB_SBASE)) $(addsuffix .o,$(addprefix $(BUILDDIR),$(PROGRAM)))
DEPENDENCY-FILES += $(foreach f, $(SOURCES_LIBUTIL) $(SOURCES_LIBUTF),\
		      $(BUILDDIR)$(dir $(f))$(patsubst %.c,%.d,$(notdir $(f))))
WFLAGS += -Wno-error=sign-compare -Wno-error=implicit-fallthrough

include $(TOPDIR)/build/build.prog.mk

$(PROGRAM).c: $(SOURCES_SBASE)
	@echo "[SH] $^ -> $@"
	echo '#include <libgen.h>'                                                                                                 > $@
	echo '#include <stdio.h>'                                                                                                     >> $@
	echo '#include <stdlib.h>'                                                                                                    >> $@
	echo '#include <string.h>'                                                                                                    >> $@
	echo '#include "util.h"'                                                                                                      >> $@
	for f in $(notdir $(SOURCES_SBASE)); do echo "int $$(echo "$${f%.c}" | sed s/-/_/g)_main(int, char **);"; done                                    >> $@
	echo 'int main(int argc, char *argv[]) { char *s = basename(argv[0]);'                                                        >> $@
	echo 'if(!strcmp(s,"$(PROGRAM)")) { argc--; argv++; s = basename(argv[0]); } if(0) ;'                                          >> $@
	echo "else if (!strcmp(s, \"[\")) return test_main(argc, argv);"                                                              >> $@
	for f in $(notdir $(SOURCES_SBASE)); do echo "else if(!strcmp(s, \"$${f%.c}\")) return $$(echo "$${f%.c}" | sed s/-/_/g)_main(argc, argv);"; done >> $@
	echo 'else { fputs("[ ", stdout);'                                                                                            >> $@
	for f in $(notdir $(SOURCES_SBASE)); do echo "fputs(\"$${f%.c} \", stdout);"; done                                                                >> $@
	echo 'putchar(0xa); }; return 0; }'                                                                                           >> $@

$(BUILDDIR)sbase/$(LIBUTIL): $(OBJ_LIBUTIL)

$(BUILDDIR)sbase/$(LIBUTF): $(OBJ_LIBUTF) 

download-here: sbase/.git

build-before: quilt-patch

install-here:
	@echo "[INSTALL] /bin/sbase/sbase-box"
	for f in $(BIN); do ln -sf $(PROGRAM) $(SYSROOT)/$(BINDIR)/"$$f"; done
	ln -sf $(PROGRAM)  $(SYSROOT)/$(BINDIR)/[

clean-here: quilt-unpatch
	rm -f $(PROGRAM).c $(PROGRAM).uelf