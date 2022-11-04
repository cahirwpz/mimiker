/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "utf.h"
#include "util.h"

typedef struct Range {
	size_t min, max;
	struct Range *next;
} Range;

static Range *list     = NULL;
static char   mode     = 0;
static char  *delim    = "\t";
static size_t delimlen = 1;
static int    nflag    = 0;
static int    sflag    = 0;

static void
insert(Range *r)
{
	Range *l, *p, *t;

	for (p = NULL, l = list; l; p = l, l = l->next) {
		if (r->max && r->max + 1 < l->min) {
			r->next = l;
			break;
		} else if (!l->max || r->min < l->max + 2) {
			l->min = MIN(r->min, l->min);
			for (p = l, t = l->next; t; p = t, t = t->next)
				if (r->max && r->max + 1 < t->min)
					break;
			l->max = (p->max && r->max) ? MAX(p->max, r->max) : 0;
			l->next = t;
			return;
		}
	}
	if (p)
		p->next = r;
	else
		list = r;
}

static void
parselist(char *str)
{
	char *s;
	size_t n = 1;
	Range *r;

	if (!*str)
		eprintf("empty list\n");
	for (s = str; *s; s++) {
		if (*s == ' ')
			*s = ',';
		if (*s == ',')
			n++;
	}
	r = ereallocarray(NULL, n, sizeof(*r));
	for (s = str; n; n--, s++) {
		r->min = (*s == '-') ? 1 : strtoul(s, &s, 10);
		r->max = (*s == '-') ? strtoul(s + 1, &s, 10) : r->min;
		r->next = NULL;
		if (!r->min || (r->max && r->max < r->min) || (*s && *s != ','))
			eprintf("bad list value\n");
		insert(r++);
	}
}

static size_t
seek(struct line *s, size_t pos, size_t *prev, size_t count)
{
	size_t n = pos - *prev, i, j;

	if (mode == 'b') {
		if (n >= s->len)
			return s->len;
		if (nflag)
			while (n && !UTF8_POINT(s->data[n]))
				n--;
		*prev += n;
		return n;
	} else if (mode == 'c') {
		for (n++, i = 0; i < s->len; i++)
			if (UTF8_POINT(s->data[i]) && !--n)
				break;
	} else {
		for (i = (count < delimlen + 1) ? 0 : delimlen; n && i < s->len; ) {
			if ((s->len - i) >= delimlen &&
			    !memcmp(s->data + i, delim, delimlen)) {
				if (!--n && count)
					break;
				i += delimlen;
				continue;
			}
			for (j = 1; j + i <= s->len && !fullrune(s->data + i, j); j++);
			i += j;
		}
	}
	*prev = pos;

	return i;
}

static void
cut(FILE *fp, const char *fname)
{
	Range *r;
	struct line s;
	static struct line line;
	static size_t size;
	size_t i, n, p;
	ssize_t len;

	while ((len = getline(&line.data, &size, fp)) > 0) {
		line.len = len;
		if (line.data[line.len - 1] == '\n')
			line.data[--line.len] = '\0';
		if (mode == 'f' && !memmem(line.data, line.len, delim, delimlen)) {
			if (!sflag) {
				fwrite(line.data, 1, line.len, stdout);
				fputc('\n', stdout);
			}
			continue;
		}
		for (i = 0, p = 1, s = line, r = list; r; r = r->next) {
			n = seek(&s, r->min, &p, i);
			s.data += n;
			s.len -= n;
			i += (mode == 'f') ? delimlen : 1;
			if (!s.len)
				break;
			if (!r->max) {
				fwrite(s.data, 1, s.len, stdout);
				break;
			}
			n = seek(&s, r->max + 1, &p, i);
			i += (mode == 'f') ? delimlen : 1;
			if (fwrite(s.data, 1, n, stdout) != n)
				eprintf("fwrite <stdout>:");
			s.data += n;
			s.len -= n;
		}
		putchar('\n');
	}
	if (ferror(fp))
		eprintf("getline %s:", fname);
}

static void
usage(void)
{
	eprintf("usage: %s -b list [-n] [file ...]\n"
	        "       %s -c list [file ...]\n"
	        "       %s -f list [-d delim] [-s] [file ...]\n",
		argv0, argv0, argv0);
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	int ret = 0;

	ARGBEGIN {
	case 'b':
	case 'c':
	case 'f':
		mode = ARGC();
		parselist(EARGF(usage()));
		break;
	case 'd':
		delim = EARGF(usage());
		if (!*delim)
			eprintf("empty delimiter\n");
		delimlen = unescape(delim);
		break;
	case 'n':
		nflag = 1;
		break;
	case 's':
		sflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if (!mode)
		usage();

	if (!argc)
		cut(stdin, "<stdin>");
	else {
		for (; *argv; argc--, argv++) {
			if (!strcmp(*argv, "-")) {
				*argv = "<stdin>";
				fp = stdin;
			} else if (!(fp = fopen(*argv, "r"))) {
				weprintf("fopen %s:", *argv);
				ret = 1;
				continue;
			}
			cut(fp, *argv);
			if (fp != stdin && fshut(fp, *argv))
				ret = 1;
		}
	}

	ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");

	return ret;
}
