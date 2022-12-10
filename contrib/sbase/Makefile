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
	pathchk\
	printf\
	sed\
	sort\
	tail\
	tee

BIN_DONT_WORK =\
	date\
	find\
	getconf\
	join\
	kill\
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

CFLAGS.sbase/basename.c += -Dmain=basename_main
CFLAGS.sbase/cat.c += -Dmain=cat_main
CFLAGS.sbase/chgrp.c += -Dmain=chgrp_main
CFLAGS.sbase/chmod.c += -Dmain=chmod_main
CFLAGS.sbase/chown.c += -Dmain=chown_main
CFLAGS.sbase/cmp.c += -Dmain=cmp_main
CFLAGS.sbase/comm.c += -Dmain=comm_main
CFLAGS.sbase/cp.c += -Dmain=cp_main
CFLAGS.sbase/cut.c += -Dmain=cut_main
CFLAGS.sbase/dd.c += -Dmain=dd_main
CFLAGS.sbase/dirname.c += -Dmain=dirname_main
CFLAGS.sbase/echo.c += -Dmain=echo_main
CFLAGS.sbase/env.c += -Dmain=env_main
CFLAGS.sbase/expand.c += -Dmain=expand_main
CFLAGS.sbase/expr.c += -Dmain=expr_main
CFLAGS.sbase/false.c += -Dmain=false_main
CFLAGS.sbase/fold.c += -Dmain=fold_main
CFLAGS.sbase/head.c += -Dmain=head_main
CFLAGS.sbase/link.c += -Dmain=link_main
CFLAGS.sbase/ln.c += -Dmain=ln_main
CFLAGS.sbase/logname.c += -Dmain=logname_main
CFLAGS.sbase/md5sum.c += -Dmain=md5sum_main
CFLAGS.sbase/mkdir.c += -Dmain=mkdir_main
CFLAGS.sbase/mkfifo.c += -Dmain=mkfifo_main
CFLAGS.sbase/mktemp.c += -Dmain=mktemp_main
CFLAGS.sbase/mv.c += -Dmain=mv_main
CFLAGS.sbase/nice.c += -Dmain=nice_main
CFLAGS.sbase/nl.c += -Dmain=nl_main
CFLAGS.sbase/nohup.c += -Dmain=nohup_main
CFLAGS.sbase/printenv.c += -Dmain=printenv_main
CFLAGS.sbase/pwd.c += -Dmain=pwd_main
CFLAGS.sbase/readlink.c += -Dmain=readlink_main
CFLAGS.sbase/renice.c += -Dmain=renice_main
CFLAGS.sbase/rev.c += -Dmain=rev_main
CFLAGS.sbase/rm.c += -Dmain=rm_main
CFLAGS.sbase/rmdir.c += -Dmain=rmdir_main
CFLAGS.sbase/seq.c += -Dmain=seq_main
CFLAGS.sbase/setsid.c += -Dmain=setsid_main
CFLAGS.sbase/sha1sum.c += -Dmain=sha1sum_main
CFLAGS.sbase/sha224sum.c += -Dmain=sha224sum_main
CFLAGS.sbase/sha256sum.c += -Dmain=sha256sum_main
CFLAGS.sbase/sha384sum.c += -Dmain=sha384sum_main
CFLAGS.sbase/sha512sum.c += -Dmain=sha512sum_main
CFLAGS.sbase/sha512-224sum.c += -Dmain=sha512_224sum_main
CFLAGS.sbase/sha512-256sum.c += -Dmain=sha512_256sum_main
CFLAGS.sbase/sleep.c += -Dmain=sleep_main
CFLAGS.sbase/split.c += -Dmain=split_main
CFLAGS.sbase/sponge.c += -Dmain=sponge_main
CFLAGS.sbase/strings.c += -Dmain=strings_main
CFLAGS.sbase/sync.c += -Dmain=sync_main
CFLAGS.sbase/test.c += -Dmain=test_main
CFLAGS.sbase/tr.c += -Dmain=tr_main
CFLAGS.sbase/true.c += -Dmain=true_main
CFLAGS.sbase/tsort.c += -Dmain=tsort_main
CFLAGS.sbase/tty.c += -Dmain=tty_main
CFLAGS.sbase/unexpand.c += -Dmain=unexpand_main
CFLAGS.sbase/uniq.c += -Dmain=uniq_main
CFLAGS.sbase/unlink.c += -Dmain=unlink_main
CFLAGS.sbase/uudecode.c += -Dmain=uudecode_main
CFLAGS.sbase/uuencode.c += -Dmain=uuencode_main
CFLAGS.sbase/wc.c += -Dmain=wc_main
CFLAGS.sbase/which.c += -Dmain=which_main
CFLAGS.sbase/whoami.c += -Dmain=whoami_main
CFLAGS.sbase/xinstall.c += -Dmain=xinstall_main
CFLAGS.sbase/yes.c += -Dmain=yes_main
CFLAGS.sbase/cal.c += -Dmain=cal_main
CFLAGS.sbase/cols.c += -Dmain=cols_main
CFLAGS.sbase/cksum.c += -Dmain=cksum_main
CFLAGS.sbase/grep.c += -Dmain=grep_main
CFLAGS.sbase/ls.c += -Dmain=ls_main
CFLAGS.sbase/od.c += -Dmain=od_main
CFLAGS.sbase/paste.c += -Dmain=paste_main
CFLAGS.sbase/pathchk.c += -Dmain=pathchk_main
CFLAGS.sbase/printf.c += -Dmain=printf_main
CFLAGS.sbase/sed.c += -Dmain=sed_main
CFLAGS.sbase/sort.c += -Dmain=sort_main
CFLAGS.sbase/tail.c += -Dmain=tail_main
CFLAGS.sbase/tee.c += -Dmain=tee_main

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