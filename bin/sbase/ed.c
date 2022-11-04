/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>
#include <fcntl.h>
#include <regex.h>
#include <unistd.h>

#include <ctype.h>
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define REGEXSIZE  100
#define LINESIZE    80
#define NUMLINES    32
#define CACHESIZ  4096
#define AFTER     0
#define BEFORE    1

typedef struct {
	char *str;
	size_t cap;
	size_t siz;
} String;

struct hline {
	off_t seek;
	char  global;
	int   next, prev;
};

struct undo {
	int curln, lastln;
	size_t nr, cap;
	struct link {
		int to1, from1;
		int to2, from2;
	} *vec;
};

static char *prompt = "*";
static regex_t *pattern;
static regmatch_t matchs[10];
static String lastre;

static int optverbose, optprompt, exstatus, optdiag = 1;
static int marks['z' - 'a'];
static int nlines, line1, line2;
static int curln, lastln, ocurln, olastln;
static jmp_buf savesp;
static char *lasterr;
static size_t idxsize, lastidx;
static struct hline *zero;
static String text;
static char savfname[FILENAME_MAX];
static char tmpname[FILENAME_MAX];
static int scratch;
static int pflag, modflag, uflag, gflag;
static size_t csize;
static String cmdline;
static char *ocmdline;
static int repidx;
static char *rhs;
static char *lastmatch;
static struct undo udata;
static int newcmd;
int eol, bol;

static void
discard(void)
{
	int c;

	if (repidx >= 0)
		return;

	/* discard until the end of the line */
	if (cmdline.siz > 0 && cmdline.str[cmdline.siz-1] == '\n')
		return;

	while ((c = getchar()) != '\n' && c != EOF)
		;
}

static void undo(void);

static void
error(char *msg)
{
	exstatus = 1;
	lasterr = msg;
	puts("?");

	if (optverbose)
		puts(msg);
	if (!newcmd)
		undo();

	discard();
	curln = ocurln;
	longjmp(savesp, 1);
}

static int
nextln(int line)
{
	++line;
	return (line > lastln) ? 0 : line;
}

static int
prevln(int line)
{
	--line;
	return (line < 0) ? lastln : line;
}

static char *
addchar(char c, String *s)
{
	size_t cap = s->cap, siz = s->siz;
	char *t = s->str;

	if (siz >= cap &&
	    (cap > SIZE_MAX - LINESIZE ||
	     (t = realloc(t, cap += LINESIZE)) == NULL))
			error("out of memory");
	t[siz++] = c;
	s->siz = siz;
	s->cap = cap;
	s->str = t;
	return t;
}

static int
input(void)
{
	int c;

	if (repidx >= 0)
		return ocmdline[repidx++];

	if ((c = getchar()) != EOF)
		addchar(c, &cmdline);
	return c;
}

static int
back(int c)
{
	if (repidx > 0) {
		--repidx;
	} else {
		ungetc(c, stdin);
		if (c != EOF)
			--cmdline.siz;
	}
	return c;
}

static int
makeline(char *s, int *off)
{
	struct hline *lp;
	size_t len;
	char c, *begin = s;

	if (lastidx >= idxsize) {
		lp = NULL;
		if (idxsize <= SIZE_MAX - NUMLINES)
		    lp = reallocarray(zero, idxsize + NUMLINES, sizeof(*lp));
		if (!lp)
			error("out of memory");
		idxsize += NUMLINES;
		zero = lp;
	}
	lp = zero + lastidx;

	if (!s) {
		lp->seek = -1;
		len = 0;
	} else {
		while ((c = *s++) != '\n')
			/* nothing */;
		len = s - begin;
		if ((lp->seek = lseek(scratch, 0, SEEK_END)) < 0 ||
		    write(scratch, begin, len) < 0) {
			error("input/output error");
		}
	}
	if (off)
		*off = len;
	++lastidx;
	return lp - zero;
}

static int
getindex(int line)
{
	struct hline *lp;
	int n;

	if (line == -1)
		line = 0;
	for (n = 0, lp = zero; n != line; n++)
		lp = zero + lp->next;

	return lp - zero;
}

static char *
gettxt(int line)
{
	static char buf[CACHESIZ];
	static off_t lasto;
	struct hline *lp;
	off_t off, block;
	ssize_t n;
	char *p;

	lp = zero + getindex(line);
	text.siz = 0;
	off = lp->seek;

	if (off == (off_t) -1)
		return addchar('\0', &text);

repeat:
	if (!csize || off < lasto || off - lasto >= csize) {
		block = off & ~(CACHESIZ-1);
		if (lseek(scratch, block, SEEK_SET) < 0 ||
		    (n = read(scratch, buf, CACHESIZ)) < 0) {
			error("input/output error");
		}
		csize = n;
		lasto = block;
	}
	for (p = buf + off - lasto; p < buf + csize && *p != '\n'; ++p) {
		++off;
		addchar(*p, &text);
	}
	if (csize && p == buf + csize)
		goto repeat;

	addchar('\n', &text);
	addchar('\0', &text);
	return text.str;
}

static void
setglobal(int i, int v)
{
	zero[getindex(i)].global = v;
}

static void
clearundo(void)
{
	free(udata.vec);
	udata.vec = NULL;
	newcmd = udata.nr = udata.cap = 0;
	modflag = 0;
}

static void
newundo(int from1, int from2)
{
	struct link *p;

	if (newcmd) {
		clearundo();
		udata.curln = ocurln;
		udata.lastln = olastln;
	}
	if (udata.nr >= udata.cap) {
		size_t siz = (udata.cap + 10) * sizeof(struct link);
		if ((p = realloc(udata.vec, siz)) == NULL)
			error("out of memory");
		udata.vec = p;
		udata.cap = udata.cap + 10;
	}
	p = &udata.vec[udata.nr++];
	p->from1 = from1;
	p->to1 = zero[from1].next;
	p->from2 = from2;
	p->to2 = zero[from2].prev;
}

/*
 * relink: to1   <- from1
 *         from2 -> to2
 */
static void
relink(int to1, int from1, int from2, int to2)
{
	newundo(from1, from2);
	zero[from1].next = to1;
	zero[from2].prev = to2;
	modflag = 1;
}

static void
undo(void)
{
	struct link *p;

	if (udata.nr == 0)
		return;
	for (p = &udata.vec[udata.nr-1]; udata.nr > 0; --p) {
		--udata.nr;
		zero[p->from1].next = p->to1;
		zero[p->from2].prev = p->to2;
	}
	free(udata.vec);
	udata.vec = NULL;
	udata.cap = 0;
	curln = udata.curln;
	lastln = udata.lastln;
}

static void
inject(char *s, int where)
{
	int off, k, begin, end;

	if (where == BEFORE) {
		begin = getindex(curln-1);
		end = getindex(nextln(curln-1));
	} else {
		begin = getindex(curln);
		end = getindex(nextln(curln));
	}
	while (*s) {
		k = makeline(s, &off);
		s += off;
		relink(k, begin, k, begin);
		relink(end, k, end, k);
		++lastln;
		++curln;
		begin = k;
	}
}

static void
clearbuf(void)
{
	if (scratch)
		close(scratch);
	remove(tmpname);
	free(zero);
	zero = NULL;
	scratch = csize = idxsize = lastidx = curln = lastln = 0;
	modflag = lastln = curln = 0;
}

static void
setscratch(void)
{
	int r, k;
	char *dir;

	clearbuf();
	clearundo();
	if ((dir = getenv("TMPDIR")) == NULL)
		dir = "/tmp";
	r = snprintf(tmpname, sizeof(tmpname), "%s/%s",
	             dir, "ed.XXXXXX");
	if (r < 0 || (size_t)r >= sizeof(tmpname))
		error("scratch filename too long");
	if ((scratch = mkstemp(tmpname)) < 0)
		error("failed to create scratch file");
	if ((k = makeline(NULL, NULL)))
		error("input/output error in scratch file");
	relink(k, k, k, k);
	clearundo();
}

static void
compile(int delim)
{
	int n, ret, c,bracket;
	static char buf[BUFSIZ];

	if (!isgraph(delim))
		error("invalid pattern delimiter");

	eol = bol = bracket = lastre.siz = 0;
	for (n = 0;; ++n) {
		if ((c = input()) == delim && !bracket)
			break;
		if (c == '^') {
			bol = 1;
		} else if (c == '$') {
			eol = 1;
		} else if (c == '\n' || c == EOF) {
			back(c);
			break;
		}

		if (c == '\\') {
			addchar(c, &lastre);
			c = input();
		} else if (c == '[') {
			bracket = 1;
		} else if (c == ']') {
			bracket = 0;
		}
		addchar(c, &lastre);
	}
	if (n == 0) {
		if (!pattern)
			error("no previous pattern");
		return;
	}
	addchar('\0', &lastre);

	if (pattern)
		regfree(pattern);
	if (!pattern && (!(pattern = malloc(sizeof(*pattern)))))
		error("out of memory");
	if ((ret = regcomp(pattern, lastre.str, REG_NEWLINE))) {
		regerror(ret, pattern, buf, sizeof(buf));
		error(buf);
	}
}

static int
match(int num)
{
	lastmatch = gettxt(num);
	return !regexec(pattern, lastmatch, 10, matchs, 0);
}

static int
rematch(int num)
{
	regoff_t off = matchs[0].rm_eo;

	if (!regexec(pattern, lastmatch + off, 10, matchs, 0)) {
		lastmatch += off;
		return 1;
	}

	return 0;
}

static int
search(int way)
{
	int i;

	i = curln;
	do {
		i = (way == '?') ? prevln(i) : nextln(i);
		if (i > 0 && match(i))
			return i;
	} while (i != curln);

	error("invalid address");
	return -1; /* not reached */
}

static void
skipblank(void)
{
	char c;

	while ((c = input()) == ' ' || c == '\t')
		/* nothing */;
	back(c);
}

static int
getnum(void)
{
	int ln, n, c;

	for (ln = 0; isdigit(c = input()); ln += n) {
		if (ln > INT_MAX/10)
			goto invalid;
		n = c - '0';
		ln *= 10;
		if (INT_MAX - ln < n)
			goto invalid;
	}
	back(c);
	return ln;

invalid:
	error("invalid address");
	return -1; /* not reached */
}

static int
linenum(int *line)
{
	int ln, c;

	skipblank();

	switch (c = input()) {
	case '.':
		ln = curln;
		break;
	case '\'':
		skipblank();
		if (!islower(c = input()))
			error("invalid mark character");
		if (!(ln = marks[c - 'a']))
			error("invalid address");
		break;
	case '$':
		ln = lastln;
		break;
	case '?':
	case '/':
		compile(c);
		ln = search(c);
		break;
	case '^':
	case '-':
	case '+':
		ln = curln;
		back(c);
		break;
	default:
		back(c);
		if (isdigit(c))
			ln = getnum();
		else
			return 0;
		break;
	}
	*line = ln;
	return 1;
}

static int
address(int *line)
{
	int ln, sign, c, num;

	if (!linenum(&ln))
		return 0;

	for (;;) {
		skipblank();
		if ((c = input()) != '+' && c != '-' && c != '^')
			break;
		sign = c == '+' ? 1 : -1;
		num = isdigit(back(input())) ? getnum() : 1;
		num *= sign;
		if (INT_MAX - ln < num)
			goto invalid;
		ln += num;
	}
	back(c);

	if (ln < 0 || ln > lastln)
		error("invalid address");
	*line = ln;
	return 1;

invalid:
	error("invalid address");
	return -1; /* not reached */
}

static void
getlst(void)
{
	int ln, c;

	if ((c = input()) == ',') {
		line1 = 1;
		line2 = lastln;
		nlines = lastln;
		return;
	} else if (c == ';') {
		line1 = curln;
		line2 = lastln;
		nlines = lastln - curln + 1;
		return;
	}
	back(c);
	line2 = curln;
	for (nlines = 0; address(&ln); ) {
		line1 = line2;
		line2 = ln;
		++nlines;

		skipblank();
		if ((c = input()) != ',' && c != ';') {
			back(c);
			break;
		}
		if (c == ';')
			curln = line2;
	}
	if (nlines > 2)
		nlines = 2;
	else if (nlines <= 1)
		line1 = line2;
}

static void
deflines(int def1, int def2)
{
	if (!nlines) {
		line1 = def1;
		line2 = def2;
	}
	if (line1 > line2 || line1 < 0 || line2 > lastln)
		error("invalid address");
}

static void
dowrite(const char *fname, int trunc)
{
	FILE *fp;
	size_t bytecount = 0;
	int i, line;

	if (!(fp = fopen(fname, (trunc) ? "w" : "a")))
		error("input/output error");

	line = curln;
	for (i = line1; i <= line2; ++i) {
		gettxt(i);
		bytecount += text.siz - 1;
		fwrite(text.str, 1, text.siz - 1, fp);
	}

	curln = line2;
	if (fclose(fp))
		error("input/output error");
	strcpy(savfname, fname);
	modflag = 0;
	curln = line;
	printf("%zu\n", bytecount);
}

static void
doread(const char *fname)
{
	size_t cnt;
	ssize_t n;
	char *p;
	FILE *aux;
	static size_t len;
	static char *s;
	static FILE *fp;

	if (fp)
		fclose(fp);
	if ((fp = fopen(fname, "r")) == NULL)
		error("cannot open input file");

	curln = line2;
	for (cnt = 0; (n = getline(&s, &len, fp)) > 0; cnt += (size_t)n) {
		if (s[n-1] != '\n') {
			if (len == SIZE_MAX || !(p = realloc(s, ++len)))
				error("out of memory");
			s = p;
			s[n-1] = '\n';
			s[n] = '\0';
		}
		inject(s, AFTER);
	}
	if (optdiag)
		printf("%zu\n", cnt);

	aux = fp;
	fp = NULL;
	if (fclose(aux))
		error("input/output error");
}

static void
doprint(void)
{
	int i, c;
	char *s, *str;

	if (line1 <= 0 || line2 > lastln)
		error("incorrect address");
	for (i = line1; i <= line2; ++i) {
		if (pflag == 'n')
			printf("%d\t", i);
		for (s = gettxt(i); (c = *s) != '\n'; ++s) {
			if (pflag != 'l')
				goto print_char;
			switch (c) {
			case '$':
				str = "\\$";
				goto print_str;
			case '\t':
				str = "\\t";
				goto print_str;
			case '\b':
				str = "\\b";
				goto print_str;
			case '\\':
				str = "\\\\";
				goto print_str;
			default:
				if (!isprint(c)) {
					printf("\\x%x", 0xFF & c);
					break;
				}
			print_char:
				putchar(c);
				break;
			print_str:
				fputs(str, stdout);
				break;
			}
		}
		if (pflag == 'l')
			fputs("$", stdout);
		putc('\n', stdout);
	}
	curln = i - 1;
}

static void
dohelp(void)
{
	if (lasterr)
		puts(lasterr);
}

static void
chkprint(int flag)
{
	char c;

	if (flag) {
		if ((c = input()) == 'p' || c == 'l' || c == 'n')
			pflag = c;
		else
			back(c);
	}
	if (input() != '\n')
		error("invalid command suffix");
}

static char *
getfname(char comm)
{
	int c;
	char *bp;
	static char fname[FILENAME_MAX];

	skipblank();
	for (bp = fname; bp < &fname[FILENAME_MAX]; *bp++ = c) {
		if ((c = input()) == EOF || c == '\n')
			break;
	}
	if (bp == fname) {
		if (savfname[0] == '\0')
			error("no current filename");
		return savfname;
	} else if (bp == &fname[FILENAME_MAX]) {
		error("file name too long");
	} else {
		*bp = '\0';
		if (savfname[0] == '\0' || comm == 'e' || comm == 'f')
			strcpy(savfname, fname);
		return fname;
	}

	return NULL; /* not reached */
}

static void
append(int num)
{
	char *s = NULL;
	size_t len = 0;

	curln = num;
	while (getline(&s, &len, stdin) > 0) {
		if (*s == '.' && s[1] == '\n')
			break;
		inject(s, AFTER);
	}
	free(s);
}

static void
delete(int from, int to)
{
	int lto, lfrom;

	if (!from)
		error("incorrect address");

	lfrom = getindex(prevln(from));
	lto = getindex(nextln(to));
	lastln -= to - from + 1;
	curln = (from > lastln) ? lastln : from;;
	relink(lto, lfrom, lto, lfrom);
}

static void
move(int where)
{
	int before, after, lto, lfrom;

	if (!line1 || (where >= line1 && where <= line2))
		error("incorrect address");

	before = getindex(prevln(line1));
	after = getindex(nextln(line2));
	lfrom = getindex(line1);
	lto = getindex(line2);
	relink(after, before, after, before);

	if (where < line1) {
		curln = where + line1 - line2 + 1;
	} else {
		curln = where;
		where -= line1 - line2 + 1;
	}
	before = getindex(where);
	after = getindex(nextln(where));
	relink(lfrom, before, lfrom, before);
	relink(after, lto, after, lto);
}

static void
join(void)
{
	int i;
	char *t, c;
	String s;

	s.str = NULL;
	s.siz = s.cap = 0;
	for (i = line1;; i = nextln(i)) {
		for (t = gettxt(i); (c = *t) != '\n'; ++t)
			addchar(*t, &s);
		if (i == line2)
			break;
	}

	addchar('\n', &s);
	addchar('\0', &s);
	delete(line1, line2);
	inject(s.str, BEFORE);
	free(s.str);
}

static void
scroll(int num)
{
	int max, ln, cnt;

	if (!line1 || line1 == lastln)
		error("incorrect address");

	ln = line1;
	max = line1 + num;
	if (max > lastln)
		max = lastln;
	for (cnt = line1; cnt < max; cnt++) {
		fputs(gettxt(ln), stdout);
		ln = nextln(ln);
	}
	curln = ln;
}

static void
copy(int where)
{

	if (!line1)
		error("incorrect address");
	curln = where;

	while (line1 <= line2) {
		inject(gettxt(line1), AFTER);
		if (line2 >= curln)
			line2 = nextln(line2);
		line1 = nextln(line1);
		if (line1 >= curln)
			line1 = nextln(line1);
	}
}

static void
quit(void)
{
	clearbuf();
	exit(exstatus);
}

static void
execsh(void)
{
	static String cmd;
	char *p;
	int c, repl = 0;

	skipblank();
	if ((c = input()) != '!') {
		back(c);
		cmd.siz = 0;
	} else if (cmd.siz) {
		--cmd.siz;
		repl = 1;
	} else {
		error("no previous command");
	}

	while ((c = input()) != EOF && c != '\n') {
		if (c == '%' && (cmd.siz == 0 || cmd.str[cmd.siz - 1] != '\\')) {
			if (savfname[0] == '\0')
				error("no current filename");
			repl = 1;
			for (p = savfname; *p; ++p)
				addchar(*p, &cmd);
		} else {
			addchar(c, &cmd);
		}
	}
	addchar('\0', &cmd);

	if (repl)
		puts(cmd.str);
	system(cmd.str);
	if (optdiag)
		puts("!");
}

static void
getrhs(int delim)
{
	int c;
	static String s;

	free(s.str);
	s.str = NULL;
	s.siz = s.cap = 0;
	while ((c = input()) != '\n' && c != EOF && c != delim)
		addchar(c, &s);
	addchar('\0', &s);
	if (c == EOF)
		error("invalid pattern delimiter");
	if (c == '\n') {
		pflag = 'p';
		back(c);
	}

	if (!strcmp("%", s.str)) {
		free(s.str);
		if (!rhs)
			error("no previous substitution");
	} else {
		free(rhs);
		rhs = s.str;
	}
	s.str = NULL;
}

static int
getnth(void)
{
	int c;

	if ((c = input()) == 'g') {
		return -1;
	} else if (isdigit(c)) {
		if (c == '0')
			return -1;
		return c - '0';
	} else {
		back(c);
		return 1;
	}
}

static void
addpre(String *s)
{
	char *p;

	for (p = lastmatch; p < lastmatch + matchs[0].rm_so; ++p)
		addchar(*p, s);
}

static void
addpost(String *s)
{
	char c, *p;

	for (p = lastmatch + matchs[0].rm_eo; (c = *p); ++p)
		addchar(c, s);
	addchar('\0', s);
}

static int
addsub(String *s, int nth, int nmatch)
{
	char *end, *q, *p, c;
	int sub;

	if (nth != nmatch && nth != -1) {
		q   = lastmatch + matchs[0].rm_so;
		end = lastmatch + matchs[0].rm_eo;
		while (q < end)
			addchar(*q++, s);
		return 0;
	}

	for (p = rhs; (c = *p); ++p) {
		switch (c) {
		case '&':
			sub = 0;
			goto copy_match;
		case '\\':
			if ((c = *++p) == '\0')
				return 1;
			if (!isdigit(c))
				goto copy_char;
			sub = c - '0';
		copy_match:
			q   = lastmatch + matchs[sub].rm_so;
			end = lastmatch + matchs[sub].rm_eo;
			while (q < end)
				addchar(*q++, s);
			break;
		default:
		copy_char:
			addchar(c, s);
			break;
		}
	}
	return 1;
}

static void
subline(int num, int nth)
{
	int i, m, changed;
	static String s;

	i = changed = s.siz = 0;
	for (m = match(num); m; m = rematch(num)) {
		addpre(&s);
		changed |= addsub(&s, nth, ++i);
		if (eol || bol)
			break;
	}
	if (!changed)
		return;
	addpost(&s);
	delete(num, num);
	curln = prevln(num);
	inject(s.str, AFTER);
}

static void
subst(int nth)
{
	int i;

	for (i = line1; i <= line2; ++i)
		subline(i, nth);
}

static void
docmd(void)
{
	char cmd;
	int rep = 0, c, line3, num, trunc;

repeat:
	skipblank();
	cmd = input();
	trunc = pflag = 0;
	switch (cmd) {
	case '&':
		skipblank();
		chkprint(0);
		if (!ocmdline)
			error("no previous command");
		rep = 1;
		repidx = 0;
		getlst();
		goto repeat;
	case '!':
		execsh();
		break;
	case EOF:
		if (cmdline.siz == 0)
			quit();
	case '\n':
		if (gflag && uflag)
			return;
		num = gflag ? curln : curln+1;
		deflines(num, num);
		pflag = 'p';
		goto print;
	case 'l':
	case 'n':
	case 'p':
		back(cmd);
		chkprint(1);
		deflines(curln, curln);
		goto print;
	case 'g':
	case 'G':
	case 'v':
	case 'V':
		error("cannot nest global commands");
	case 'H':
		if (nlines > 0)
			goto unexpected;
		chkprint(0);
		optverbose ^= 1;
		break;
	case 'h':
		if (nlines > 0)
			goto unexpected;
		chkprint(0);
		dohelp();
		break;
	case 'w':
		trunc = 1;
	case 'W':
		deflines(nextln(0), lastln);
		dowrite(getfname(cmd), trunc);
		break;
	case 'r':
		if (nlines > 1)
			goto bad_address;
		deflines(lastln, lastln);
		doread(getfname(cmd));
		break;
	case 'd':
		chkprint(1);
		deflines(curln, curln);
		delete(line1, line2);
		break;
	case '=':
		if (nlines > 1)
			goto bad_address;
		chkprint(1);
		deflines(lastln, lastln);
		printf("%d\n", line1);
		break;
	case 'u':
		if (nlines > 0)
			goto bad_address;
		chkprint(1);
		if (udata.nr == 0)
			error("nothing to undo");
		undo();
		break;
	case 's':
		deflines(curln, curln);
		c = input();
		compile(c);
		getrhs(c);
		num = getnth();
		chkprint(1);
		subst(num);
		break;
	case 'i':
		if (nlines > 1)
			goto bad_address;
		chkprint(1);
		deflines(curln, curln);
		if (!line1)
			line1++;
		append(prevln(line1));
		break;
	case 'a':
		if (nlines > 1)
			goto bad_address;
		chkprint(1);
		deflines(curln, curln);
		append(line1);
		break;
	case 'm':
		deflines(curln, curln);
		if (!address(&line3))
			line3 = curln;
		chkprint(1);
		move(line3);
		break;
	case 't':
		deflines(curln, curln);
		if (!address(&line3))
			line3 = curln;
		chkprint(1);
		copy(line3);
		break;
	case 'c':
		chkprint(1);
		deflines(curln, curln);
		delete(line1, line2);
		append(prevln(line1));
		break;
	case 'j':
		chkprint(1);
		deflines(curln, curln+1);
		if (line1 != line2 && curln != 0)
	      		join();
		break;
	case 'z':
		if (nlines > 1)
			goto bad_address;
		if (isdigit(back(input())))
			num = getnum();
		else
			num = 24;
		chkprint(1);
		scroll(num);
		break;
	case 'k':
		if (nlines > 1)
			goto bad_address;
		if (!islower(c = input()))
			error("invalid mark character");
		chkprint(1);
		deflines(curln, curln);
		marks[c - 'a'] = line1;
		break;
	case 'P':
		if (nlines > 0)
			goto unexpected;
		chkprint(1);
		optprompt ^= 1;
		break;
	case 'Q':
		modflag = 0;
	case 'q':
		if (nlines > 0)
			goto unexpected;
		if (modflag)
			goto modified;
		quit();
		break;
	case 'f':
		if (nlines > 0)
			goto unexpected;
		if (back(input()) != '\n')
			getfname(cmd);
		else
			puts(savfname);
		chkprint(0);
		break;
	case 'E':
		modflag = 0;
	case 'e':
		if (nlines > 0)
			goto unexpected;
		if (modflag)
			goto modified;
		getfname(cmd);
		setscratch();
		deflines(curln, curln);
		doread(savfname);
		clearundo();
		break;
	default:
		error("unknown command");
	bad_address:
		error("invalid address");
	modified:
		modflag = 0;
		error("warning: file modified");
	unexpected:
		error("unexpected address");
	}

	if (!pflag)
		goto save_last_cmd;

	line1 = line2 = curln;
print:
	doprint();

save_last_cmd:
	if (!uflag)
		repidx = 0;
	if (rep)
		return;
	free(ocmdline);
	addchar('\0', &cmdline);
	if ((ocmdline = strdup(cmdline.str)) == NULL)
		error("out of memory");
}

static int
chkglobal(void)
{
	int delim, c, dir, i, v;

	uflag = 1;
	gflag = 0;
	skipblank();

	switch (c = input()) {
	case 'g':
		uflag = 0;
	case 'G':
		dir = 1;
		break;
	case 'v':
		uflag = 0;
	case 'V':
		dir = 0;
		break;
	default:
		back(c);
		return 0;
	}
	gflag = 1;
	deflines(nextln(0), lastln);
	delim = input();
	compile(delim);

	for (i = 1; i <= lastln; ++i) {
		if (i >= line1 && i <= line2)
			v = match(i) == dir;
		else
			v = 0;
		setglobal(i, v);
	}

	return 1;
}

static void
doglobal(void)
{
	int cnt, ln, k;

	skipblank();
	cmdline.siz = 0;
	gflag = 1;
	if (uflag)
		chkprint(0);

	ln = line1;
	for (cnt = 0; cnt < lastln; ) {
		k = getindex(ln);
		if (zero[k].global) {
			zero[k].global = 0;
			curln = ln;
			nlines = 0;
			if (uflag) {
				line1 = line2 = ln;
				pflag = 0;
				doprint();
			}
			getlst();
			docmd();
		} else {
			cnt++;
			ln = nextln(ln);
		}
	}
	discard();   /* cover the case of not matching anything */
}

static void
usage(void)
{
	eprintf("usage: %s [-s] [-p] [file]\n", argv0);
}

static void
sigintr(int n)
{
	signal(SIGINT, sigintr);
	error("interrupt");
}

static void
sighup(int dummy)
{
	int n;
	char *home = getenv("HOME"), fname[FILENAME_MAX];

	if (modflag) {
		line1 = nextln(0);
		line2 = lastln;
		if (!setjmp(savesp)) {
			dowrite("ed.hup", 1);
		} else if (home && !setjmp(savesp)) {
			n = snprintf(fname,
			             sizeof(fname), "%s/%s", home, "ed.hup");
			if (n < sizeof(fname) && n > 0)
				dowrite(fname, 1);
		}
	}
	exstatus = 1;
	quit();
}

static void
edit(void)
{
	for (;;) {
		newcmd = 1;
		ocurln = curln;
		olastln = lastln;
		cmdline.siz = 0;
		repidx = -1;
		if (optprompt) {
			fputs(prompt, stdout);
			fflush(stdout);
		}
		getlst();
		chkglobal() ? doglobal() : docmd();
	}
}

static void
init(char *fname)
{
	size_t len;

	setscratch();
	if (!fname)
		return;
	if ((len = strlen(fname)) >= FILENAME_MAX || len == 0)
		error("incorrect filename");
	memcpy(savfname, fname, len);
	doread(fname);
	clearundo();
}

int
main(int argc, char *argv[])
{
	ARGBEGIN {
	case 'p':
		prompt = EARGF(usage());
		optprompt = 1;
		break;
	case 's':
		optdiag = 0;
		break;
	default:
		usage();
	} ARGEND

	if (argc > 1)
		usage();

	if (!setjmp(savesp)) {
		signal(SIGINT, sigintr);
		signal(SIGHUP, sighup);
		signal(SIGQUIT, SIG_IGN);
		init(*argv);
	}
	edit();

	/* not reached */
	return 0;
}
