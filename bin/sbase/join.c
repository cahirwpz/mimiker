/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "utf.h"
#include "util.h"

enum {
	INIT = 1,
	GROW = 2,
};

enum {
	EXPAND = 0,
	RESET  = 1,
};

enum { FIELD_ERROR = -2, };

struct field {
	char *s;
	size_t len;
};

struct jline {
	struct line text;
	size_t nf;
	size_t maxf;
	struct field *fields;
};

struct spec {
	size_t fileno;
	size_t fldno;
};

struct outlist {
	size_t ns;
	size_t maxs;
	struct spec **specs;
};

struct span {
	size_t nl;
	size_t maxl;
	struct jline **lines;
};

static char *sep = NULL;
static char *replace = NULL;
static const char defaultofs = ' ';
static const int jfield = 1;            /* POSIX default join field */
static int unpairsa = 0, unpairsb = 0;
static int oflag = 0;
static int pairs = 1;
static size_t seplen;
static struct outlist output;

static void
usage(void)
{
	eprintf("usage: %s [-1 field] [-2 field] [-o list] [-e string] "
	        "[-a | -v fileno] [-t delim] file1 file2\n", argv0);
}

static void
prfield(struct field *fp)
{
	if (fwrite(fp->s, 1, fp->len, stdout) != fp->len)
		eprintf("fwrite:");
}

static void
prsep(void)
{
	if (sep)
		fwrite(sep, 1, seplen, stdout);
	else
		putchar(defaultofs);
}

static void
swaplines(struct jline *la, struct jline *lb)
{
	struct jline tmp;

	tmp = *la;
	*la = *lb;
	*lb = tmp;
}

static void
prjoin(struct jline *la, struct jline *lb, size_t jfa, size_t jfb)
{
	struct spec *sp;
	struct field *joinfield;
	size_t i;

	if (jfa >= la->nf || jfb >= lb->nf)
		return;

	joinfield = &la->fields[jfa];

	if (oflag) {
		for (i = 0; i < output.ns; i++) {
			sp = output.specs[i];

			if (sp->fileno == 1) {
				if (sp->fldno < la->nf)
					prfield(&la->fields[sp->fldno]);
				else if (replace)
					fputs(replace, stdout);
			} else if (sp->fileno == 2) {
				if (sp->fldno < lb->nf)
					prfield(&lb->fields[sp->fldno]);
				else if (replace)
					fputs(replace, stdout);
			} else if (sp->fileno == 0) {
				prfield(joinfield);
			}

			if (i < output.ns - 1)
				prsep();
		}
	} else {
		prfield(joinfield);
		prsep();

		for (i = 0; i < la->nf; i++) {
			if (i != jfa) {
				prfield(&la->fields[i]);
				prsep();
			}
		}
		for (i = 0; i < lb->nf; i++) {
			if (i != jfb) {
				prfield(&lb->fields[i]);
				if (i < lb->nf - 1)
					prsep();
			}
		}
	}
	putchar('\n');
}

static void
prline(struct jline *lp)
{
	if (fwrite(lp->text.data, 1, lp->text.len, stdout) != lp->text.len)
		eprintf("fwrite:");
	putchar('\n');
}

static int
jlinecmp(struct jline *la, struct jline *lb, size_t jfa, size_t jfb)
{
	int status;

	/* return FIELD_ERROR if both lines are short */
	if (jfa >= la->nf) {
		status = (jfb >= lb->nf) ? FIELD_ERROR : -1;
	} else if (jfb >= lb->nf) {
		status = 1;
	} else {
		status = memcmp(la->fields[jfa].s, lb->fields[jfb].s,
		                MAX(la->fields[jfa].len, lb->fields[jfb].len));
		LIMIT(status, -1, 1);
	}

	return status;
}

static void
addfield(struct jline *lp, char *sp, size_t len)
{
	if (lp->nf >= lp->maxf) {
		lp->fields = ereallocarray(lp->fields, (GROW * lp->maxf),
		        sizeof(struct field));
		lp->maxf *= GROW;
	}
	lp->fields[lp->nf].s = sp;
	lp->fields[lp->nf].len = len;
	lp->nf++;
}

static void
prspanjoin(struct span *spa, struct span *spb, size_t jfa, size_t jfb)
{
	size_t i, j;

	for (i = 0; i < (spa->nl - 1); i++)
		for (j = 0; j < (spb->nl - 1); j++)
			prjoin(spa->lines[i], spb->lines[j], jfa, jfb);
}

static struct jline *
makeline(char *s, size_t len)
{
	struct jline *lp;
	char *tmp;
	size_t i, end;

	if (s[len - 1] == '\n')
		s[--len] = '\0';

	lp = ereallocarray(NULL, INIT, sizeof(struct jline));
	lp->text.data = s;
	lp->text.len = len;
	lp->fields = ereallocarray(NULL, INIT, sizeof(struct field));
	lp->nf = 0;
	lp->maxf = INIT;

	for (i = 0; i < lp->text.len && isblank(lp->text.data[i]); i++)
		;
	while (i < lp->text.len) {
		if (sep) {
			if ((lp->text.len - i) < seplen ||
			    !(tmp = memmem(lp->text.data + i,
			                   lp->text.len - i, sep, seplen))) {
				goto eol;
			}
			end = tmp - lp->text.data;
			addfield(lp, lp->text.data + i, end - i);
			i = end + seplen;
		} else {
			for (end = i; !(isblank(lp->text.data[end])); end++) {
				if (end + 1 == lp->text.len)
					goto eol;
			}
			addfield(lp, lp->text.data + i, end - i);
			for (i = end; isblank(lp->text.data[i]); i++)
				;
		}
	}
eol:
	addfield(lp, lp->text.data + i, lp->text.len - i);

	return lp;
}

static int
addtospan(struct span *sp, FILE *fp, int reset)
{
	char *newl = NULL;
	ssize_t len;
	size_t size = 0;

	if ((len = getline(&newl, &size, fp)) < 0) {
		if (ferror(fp))
			eprintf("getline:");
		else
			return 0;
	}

	if (reset)
		sp->nl = 0;

	if (sp->nl >= sp->maxl) {
		sp->lines = ereallocarray(sp->lines, (GROW * sp->maxl),
		        sizeof(struct jline *));
		sp->maxl *= GROW;
	}

	sp->lines[sp->nl] = makeline(newl, len);
	sp->nl++;
	return 1;
}

static void
initspan(struct span *sp)
{
	sp->nl = 0;
	sp->maxl = INIT;
	sp->lines = ereallocarray(NULL, INIT, sizeof(struct jline *));
}

static void
freespan(struct span *sp)
{
	size_t i;

	for (i = 0; i < sp->nl; i++) {
		free(sp->lines[i]->fields);
		free(sp->lines[i]->text.data);
	}
	free(sp->lines);
}

static void
initolist(struct outlist *olp)
{
	olp->ns = 0;
	olp->maxs = 1;
	olp->specs = ereallocarray(NULL, INIT, sizeof(struct spec *));
}

static void
addspec(struct outlist *olp, struct spec *sp)
{
	if (olp->ns >= olp->maxs) {
		olp->specs = ereallocarray(olp->specs, (GROW * olp->maxs),
		        sizeof(struct spec *));
		olp->maxs *= GROW;
	}
	olp->specs[olp->ns] = sp;
	olp->ns++;
}

static struct spec *
makespec(char *s)
{
	struct spec *sp;
	int fileno;
	size_t fldno;

	if (!strcmp(s, "0")) {   /* join field must be 0 and nothing else */
		fileno = 0;
		fldno = 0;
	} else if ((s[0] == '1' || s[0] == '2') && s[1] == '.') {
		fileno = s[0] - '0';
		fldno = estrtonum(&s[2], 1, MIN(LLONG_MAX, SIZE_MAX)) - 1;
	} else {
		eprintf("%s: invalid format\n", s);
	}

	sp = ereallocarray(NULL, INIT, sizeof(struct spec));
	sp->fileno = fileno;
	sp->fldno = fldno;
	return sp;
}

static void
makeolist(struct outlist *olp, char *s)
{
	char *item, *sp;
	sp = s;

	while (sp) {
		item = sp;
		sp = strpbrk(sp, ", \t");
		if (sp)
			*sp++ = '\0';
		addspec(olp, makespec(item));
	}
}

static void
freespecs(struct outlist *olp)
{
	size_t i;

	for (i = 0; i < olp->ns; i++)
		free(olp->specs[i]);
}

static void
join(FILE *fa, FILE *fb, size_t jfa, size_t jfb)
{
	struct span spa, spb;
	int cmp, eofa, eofb;

	initspan(&spa);
	initspan(&spb);
	cmp = eofa = eofb = 0;

	addtospan(&spa, fa, RESET);
	addtospan(&spb, fb, RESET);

	while (spa.nl && spb.nl) {
		if ((cmp = jlinecmp(spa.lines[0], spb.lines[0], jfa, jfb)) < 0) {
			if (unpairsa)
				prline(spa.lines[0]);
			if (!addtospan(&spa, fa, RESET)) {
				if (unpairsb) {    /* a is EOF'd; print the rest of b */
					do
						prline(spb.lines[0]);
					while (addtospan(&spb, fb, RESET));
				}
				eofa = eofb = 1;
			} else {
				continue;
			}
		} else if (cmp > 0) {
			if (unpairsb)
				prline(spb.lines[0]);
			if (!addtospan(&spb, fb, RESET)) {
				if (unpairsa) {    /* b is EOF'd; print the rest of a */
					do
						prline(spa.lines[0]);
					while (addtospan(&spa, fa, RESET));
				}
				eofa = eofb = 1;
			} else {
				continue;
			}
		} else if (cmp == 0) {
			/* read all consecutive matching lines from a */
			do {
				if (!addtospan(&spa, fa, EXPAND)) {
					eofa = 1;
					spa.nl++;
					break;
				}
			} while (jlinecmp(spa.lines[spa.nl-1], spb.lines[0], jfa, jfb) == 0);

			/* read all consecutive matching lines from b */
			do {
				if (!addtospan(&spb, fb, EXPAND)) {
					eofb = 1;
					spb.nl++;
					break;
				}
			} while (jlinecmp(spa.lines[0], spb.lines[spb.nl-1], jfa, jfb) == 0);

			if (pairs)
				prspanjoin(&spa, &spb, jfa, jfb);

		} else {      /* FIELD_ERROR: both lines lacked join fields */
			if (unpairsa)
				prline(spa.lines[0]);
			if (unpairsb)
				prline(spb.lines[0]);
			eofa = addtospan(&spa, fa, RESET) ? 0 : 1;
			eofb = addtospan(&spb, fb, RESET) ? 0 : 1;
			if (!eofa && !eofb)
				continue;
		}

		if (eofa) {
			spa.nl = 0;
		} else {
			swaplines(spa.lines[0], spa.lines[spa.nl - 1]);   /* ugly */
			spa.nl = 1;
		}

		if (eofb) {
			spb.nl = 0;
		} else {
			swaplines(spb.lines[0], spb.lines[spb.nl - 1]);   /* ugly */
			spb.nl = 1;
		}
	}
	freespan(&spa);
	freespan(&spb);
}


int
main(int argc, char *argv[])
{
	size_t jf[2] = { jfield, jfield, };
	FILE *fp[2];
	int ret = 0, n;
	char *fno;

	ARGBEGIN {
	case '1':
		jf[0] = estrtonum(EARGF(usage()), 1, MIN(LLONG_MAX, SIZE_MAX));
		break;
	case '2':
		jf[1] = estrtonum(EARGF(usage()), 1, MIN(LLONG_MAX, SIZE_MAX));
		break;
	case 'a':
		fno = EARGF(usage());
		if (strcmp(fno, "1") == 0)
			unpairsa = 1;
		else if (strcmp(fno, "2") == 0)
			unpairsb = 1;
		else
			usage();
		break;
	case 'e':
		replace = EARGF(usage());
		break;
	case 'o':
		oflag = 1;
		initolist(&output);
		makeolist(&output, EARGF(usage()));
		break;
	case 't':
		sep = EARGF(usage());
		break;
	case 'v':
		pairs = 0;
		fno = EARGF(usage());
		if (strcmp(fno, "1") == 0)
			unpairsa = 1;
		else if (strcmp(fno, "2") == 0)
			unpairsb = 1;
		else
			usage();
		break;
	default:
		usage();
	} ARGEND

	if (sep)
		seplen = unescape(sep);

	if (argc != 2)
		usage();

	for (n = 0; n < 2; n++) {
		if (!strcmp(argv[n], "-")) {
			argv[n] = "<stdin>";
			fp[n] = stdin;
		} else if (!(fp[n] = fopen(argv[n], "r"))) {
			eprintf("fopen %s:", argv[n]);
		}
	}

	jf[0]--;
	jf[1]--;

	join(fp[0], fp[1], jf[0], jf[1]);

	if (oflag)
		freespecs(&output);

	if (fshut(fp[0], argv[0]) | (fp[0] != fp[1] && fshut(fp[1], argv[1])) |
	    fshut(stdout, "<stdout>"))
		ret = 2;

	return ret;
}
