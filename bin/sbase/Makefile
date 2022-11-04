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
	cp\
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
	yes

BIN_DISABLED =\
	cal\
	chroot\
	cksum\
	cols\
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
	od\
	paste\
	pathchk\
	printf\
	sed\
	sort\
	tail\
	tftp\
	tar\
	tee\
	time\
	touch\
	xargs\

LIBUTFOBJ = $(LIBUTFSRC:.c=.o)
LIBUTILOBJ = $(LIBUTILSRC:.c=.o)
OBJ = $(BIN:=.o) $(LIBUTFOBJ) $(LIBUTILOBJ)
SRC = $(BIN:=.c)
MAN = $(BIN:=.1)

all: $(OBJ) $(BIN)

include $(TOPDIR)/build/flags.user.mk
include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk

# sbase version
VERSION = 0.0

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# for NetBSD add -D_NETBSD_SOURCE
# -lrt might be needed on some systems
CPPFLAGS += -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_FILE_OFFSET_BITS=64


$(BIN): $(LIB) $(@:=.o)

$(OBJ): $(HDR)

.o:
	$(CC) $(LDFLAGS) -o $@ $< $(LIB)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(LIBUTF): $(LIBUTFOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

$(LIBUTIL): $(LIBUTILOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

getconf.o: getconf.h

getconf.h: getconf.sh
	./getconf.sh > $@

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin
	cd $(DESTDIR)$(PREFIX)/bin && ln -f test [ && chmod 755 $(BIN)
	mv -f $(DESTDIR)$(PREFIX)/bin/xinstall $(DESTDIR)$(PREFIX)/bin/install
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	for m in $(MAN); do sed "s/^\.Os sbase/.Os sbase $(VERSION)/g" < "$$m" > $(DESTDIR)$(MANPREFIX)/man1/"$$m"; done
	cd $(DESTDIR)$(MANPREFIX)/man1 && chmod 644 $(MAN)
	mv -f $(DESTDIR)$(MANPREFIX)/man1/xinstall.1 $(DESTDIR)$(MANPREFIX)/man1/install.1

uninstall:
	cd $(DESTDIR)$(PREFIX)/bin && rm -f $(BIN) [ install
	cd $(DESTDIR)$(MANPREFIX)/man1 && rm -f $(MAN) install.1

dist: clean
	mkdir -p sbase-$(VERSION)
	cp -r LICENSE Makefile README TODO $(SRC) $(MAN) libutf libutil $(HDR) sbase-$(VERSION)
	tar -cf sbase-$(VERSION).tar sbase-$(VERSION)
	gzip sbase-$(VERSION).tar
	rm -rf sbase-$(VERSION)

sbase-box: $(LIB) $(SRC) getconf.h
	mkdir -p build
	cp $(HDR) build
	cp getconf.h build
	for f in $(SRC); do sed "s/^main(/$$(echo "$${f%.c}" | sed s/-/_/g)_&/" < $$f > build/$$f; done
	echo '#include <libgen.h>'                                                                                                     > build/$@.c
	echo '#include <stdio.h>'                                                                                                     >> build/$@.c
	echo '#include <stdlib.h>'                                                                                                    >> build/$@.c
	echo '#include <string.h>'                                                                                                    >> build/$@.c
	echo '#include "util.h"'                                                                                                      >> build/$@.c
	for f in $(SRC); do echo "int $$(echo "$${f%.c}" | sed s/-/_/g)_main(int, char **);"; done                                    >> build/$@.c
	echo 'int main(int argc, char *argv[]) { char *s = basename(argv[0]);'                                                        >> build/$@.c
	echo 'if(!strcmp(s,"sbase-box")) { argc--; argv++; s = basename(argv[0]); } if(0) ;'                                          >> build/$@.c
	echo "else if (!strcmp(s, \"install\")) return xinstall_main(argc, argv);"                                                    >> build/$@.c
	echo "else if (!strcmp(s, \"[\")) return test_main(argc, argv);"                                                              >> build/$@.c
	for f in $(SRC); do echo "else if(!strcmp(s, \"$${f%.c}\")) return $$(echo "$${f%.c}" | sed s/-/_/g)_main(argc, argv);"; done >> build/$@.c
	echo 'else { fputs("[ ", stdout);'                                                                                            >> build/$@.c
	for f in $(SRC); do echo "fputs(\"$${f%.c} \", stdout);"; done                                                                >> build/$@.c
	echo 'putchar(0xa); }; return 0; }'                                                                                           >> build/$@.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ build/*.c $(LIB)
	rm -r build

sbase-box-install: sbase-box
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f sbase-box $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/sbase-box
	for f in $(BIN); do ln -sf sbase-box $(DESTDIR)$(PREFIX)/bin/"$$f"; done
	ln -sf sbase-box $(DESTDIR)$(PREFIX)/bin/[
	mv -f $(DESTDIR)$(PREFIX)/bin/xinstall $(DESTDIR)$(PREFIX)/bin/install
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	for m in $(MAN); do sed "s/^\.Os sbase/.Os sbase $(VERSION)/g" < "$$m" > $(DESTDIR)$(MANPREFIX)/man1/"$$m"; done
	cd $(DESTDIR)$(MANPREFIX)/man1 && chmod 644 $(MAN)
	mv -f $(DESTDIR)$(MANPREFIX)/man1/xinstall.1 $(DESTDIR)$(MANPREFIX)/man1/install.1

sbase-box-uninstall: uninstall
	cd $(DESTDIR)$(PREFIX)/bin && rm -f sbase-box

clean-here:
	rm -f $(BIN) $(OBJ) $(LIB) sbase-box sbase-$(VERSION).tar.gz
	rm -f getconf.h

.gitignore:
	{ printf '*.o\n' ; printf '/%s\n' getconf.h $(LIB) $(BIN) ; } > $@

.PHONY: all install uninstall dist sbase-box-install sbase-box-uninstall clean .gitignore
