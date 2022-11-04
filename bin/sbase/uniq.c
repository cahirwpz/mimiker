/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "util.h"

static const char *countfmt = "";
static int dflag = 0;
static int uflag = 0;
static int fskip = 0;
static int sskip = 0;

static struct line prevl;
static ssize_t prevoff    = -1;
static long prevlinecount = 0;

static size_t
uniqskip(struct line *l)
{
	size_t i;
	int f = fskip, s = sskip;

	for (i = 0; i < l->len && f; --f) {
		while (isblank(l->data[i]))
			i++;
		while (i < l->len && !isblank(l->data[i]))
			i++;
	}
	for (; s && i < l->len && l->data[i] != '\n'; --s, i++)
		;

	return i;
}

static void
uniqline(FILE *ofp, struct line *l)
{
	size_t loff;

	if (l) {
		loff = uniqskip(l);

		if (prevoff >= 0 && (l->len - loff) == (prevl.len - prevoff) &&
		    !memcmp(l->data + loff, prevl.data + prevoff, l->len - loff)) {
			++prevlinecount;
			return;
		}
	}

	if (prevoff >= 0) {
		if ((prevlinecount == 1 && !dflag) ||
		    (prevlinecount != 1 && !uflag)) {
			if (*countfmt)
				fprintf(ofp, countfmt, prevlinecount);
			fwrite(prevl.data, 1, prevl.len, ofp);
		}
		prevoff = -1;
	}

	if (l) {
		if (!prevl.data || l->len >= prevl.len) {
			prevl.data = erealloc(prevl.data, l->len);
		}
		prevl.len = l->len;
		memcpy(prevl.data, l->data, prevl.len);
		prevoff = loff;
	}
	prevlinecount = 1;
}

static void
uniq(FILE *fp, FILE *ofp)
{
	static struct line line;
	static size_t size;
	ssize_t len;

	while ((len = getline(&line.data, &size, fp)) > 0) {
		line.len = len;
		uniqline(ofp, &line);
	}
}

static void
uniqfinish(FILE *ofp)
{
	uniqline(ofp, NULL);
}

static void
usage(void)
{
	eprintf("usage: %s [-c] [-d | -u] [-f fields] [-s chars]"
	        " [input [output]]\n", argv0);
}

int
main(int argc, char *argv[])
{
	FILE *fp[2] = { stdin, stdout };
	int ret = 0, i;
	char *fname[2] = { "<stdin>", "<stdout>" };

	ARGBEGIN {
	case 'c':
		countfmt = "%7ld ";
		break;
	case 'd':
		dflag = 1;
		break;
	case 'u':
		uflag = 1;
		break;
	case 'f':
		fskip = estrtonum(EARGF(usage()), 0, INT_MAX);
		break;
	case 's':
		sskip = estrtonum(EARGF(usage()), 0, INT_MAX);
		break;
	default:
		usage();
	} ARGEND

	if (argc > 2)
		usage();

	for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-")) {
			fname[i] = argv[i];
			if (!(fp[i] = fopen(argv[i], (i == 0) ? "r" : "w")))
				eprintf("fopen %s:", argv[i]);
		}
	}

	uniq(fp[0], fp[1]);
	uniqfinish(fp[1]);

	ret |= fshut(fp[0], fname[0]) | fshut(fp[1], fname[1]);

	return ret;
}
