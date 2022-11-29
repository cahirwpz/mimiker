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

LIB = $(LIBUTF) $(LIBUTIL)

BIN =\
	basename\
	cat\
	chgrp\
	chmod\
	chown\
	cmp\
	comm\
	cut\
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
	nice\
	nl\
	nohup\
	printenv\
	pwd\
	readlink\
	renice\
	rev\
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
	yes

BIN_DISABLED =\
	cal\
	chroot\
	cksum\
	cols\
	cp\
	cron\
	date\
	dd\
	du\
	flock\
	find\
	ed\
	getconf\
	grep\
	hostname\
	join\
	kill\
	logger\
	ls\
	mknod\
	mv\
	od\
	paste\
	pathchk\
	printf\
	rm\
	sed\
	sort\
	sponge\
	tail\
	tftp\
	tar\
	tee\
	time\
	touch\
	xargs\
	xinstall\

LIBUTFOBJ = $(LIBUTFSRC:.c=.o)
LIBUTILOBJ = $(LIBUTILSRC:.c=.o)
OBJ = $(BIN:=.o) $(LIBUTFOBJ) $(LIBUTILOBJ)
SRC = $(BIN:=.c)
INSTALLDIR = $(SYSROOT)/$(DIR)
PROGRAM = sbase-box

$(PROGRAM).c: $(SRC)
	@echo "[SBASE] $^ -> $@"
	echo '#include <libgen.h>'                                                                                                 > $@
	echo '#include <stdio.h>'                                                                                                     >> $@
	echo '#include <stdlib.h>'                                                                                                    >> $@
	echo '#include <string.h>'                                                                                                    >> $@
	echo '#include "util.h"'                                                                                                      >> $@
	for f in $(SRC); do echo "int $$(echo "$${f%.c}" | sed s/-/_/g)_main(int, char **);"; done                                    >> $@
	echo 'int main(int argc, char *argv[]) { char *s = basename(argv[0]);'                                                        >> $@
	echo 'if(!strcmp(s,"$(PROGRAM)")) { argc--; argv++; s = basename(argv[0]); } if(0) ;'                                          >> $@
	echo "else if (!strcmp(s, \"[\")) return test_main(argc, argv);"                                                              >> $@
	for f in $(SRC); do echo "else if(!strcmp(s, \"$${f%.c}\")) return $$(echo "$${f%.c}" | sed s/-/_/g)_main(argc, argv);"; done >> $@
	echo 'else { fputs("[ ", stdout);'                                                                                            >> $@
	for f in $(SRC); do echo "fputs(\"$${f%.c} \", stdout);"; done                                                                >> $@
	echo 'putchar(0xa); }; return 0; }'                                                                                           >> $@

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

$(LIBUTF): $(LIBUTFOBJ)
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rc $@ $?
	$(RANLIB) $@

$(LIBUTIL): $(LIBUTILOBJ)
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rc $@ $?
	$(RANLIB) $@

$(PROGRAM): $(PROGRAM).o $(BIN:=.o) $(LIB)
	@echo "[LD] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(LD) $(LDFLAGS) -o $(PROGRAM) $^ $(LDLIBS)

build-here: $(PROGRAM)

install-here:
	@echo "[INSTALL] $(DIR)/$(PROGRAM) -> $(INSTALLDIR)/$(PROGRAM)"
	install -D $(PROGRAM) --mode=0755 $(INSTALLDIR)/$(PROGRAM)
	for f in $(BIN); do ln -sf $(PROGRAM) $(INSTALLDIR)/"$$f"; done
	ln -sf $(PROGRAM) $(INSTALLDIR)/[

clean-here:
	rm -f $(OBJ) $(LIB) $(PROGRAM) $(PROGRAM).*

.PHONY: all install uninstall dist clean
