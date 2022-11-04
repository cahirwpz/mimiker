/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "util.h"
#include "utf.h"

static int    bflag = 0;
static int    sflag = 0;
static size_t width = 80;

static void
foldline(struct line *l, const char *fname) {
	size_t i, col, last, spacesect, len;
	Rune r;
	int runelen;

	for (i = 0, last = 0, col = 0, spacesect = 0; i < l->len; i += runelen) {
		if (col >= width && ((l->data[i] != '\r' && l->data[i] != '\b') || bflag)) {
			if (bflag && col > width)
				i -= runelen;	/* never split a character */
			len = ((sflag && spacesect) ? spacesect : i) - last;
			if (fwrite(l->data + last, 1, len, stdout) != len)
				eprintf("fwrite <stdout>:");
			if (l->data[i] != '\n')
				putchar('\n');
			if (sflag && spacesect)
				i = spacesect;
			last = i;
			col = 0;
			spacesect = 0;
		}
		runelen = charntorune(&r, l->data + i, l->len - i);
		if (!runelen || r == Runeerror)
			eprintf("charntorune: %s: invalid utf\n", fname);
		if (sflag && isblankrune(r))
			spacesect = i + runelen;
		if (!bflag && iscntrl(l->data[i])) {
			switch(l->data[i]) {
			case '\b':
				col -= (col > 0);
				break;
			case '\r':
				col = 0;
				break;
			case '\t':
				col += (8 - (col % 8));
				if (col >= width)
					i--;
				break;
			}
		} else {
			col += bflag ? runelen : 1;
		}
	}
	if (l->len - last)
		fwrite(l->data + last, 1, l->len - last, stdout);
}

static void
fold(FILE *fp, const char *fname)
{
	static struct line line;
	static size_t size = 0;
	ssize_t len;

	while ((len = getline(&line.data, &size, fp)) > 0) {
		line.len = len;
		foldline(&line, fname);
	}
	if (ferror(fp))
		eprintf("getline %s:", fname);
}

static void
usage(void)
{
	eprintf("usage: %s [-bs] [-w num | -num] [FILE ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	int ret = 0;

	ARGBEGIN {
	case 'b':
		bflag = 1;
		break;
	case 's':
		sflag = 1;
		break;
	case 'w':
		width = estrtonum(EARGF(usage()), 1, MIN(LLONG_MAX, SIZE_MAX));
		break;
	ARGNUM:
		if (!(width = ARGNUMF()))
			eprintf("illegal width value, too small\n");
		break;
	default:
		usage();
	} ARGEND

	if (!argc) {
		fold(stdin, "<stdin>");
	} else {
		for (; *argv; argc--, argv++) {
			if (!strcmp(*argv, "-")) {
				*argv = "<stdin>";
				fp = stdin;
			} else if (!(fp = fopen(*argv, "r"))) {
				weprintf("fopen %s:", *argv);
				ret = 1;
				continue;
			}
			fold(fp, *argv);
			if (fp != stdin && fshut(fp, *argv))
				ret = 1;
		}
	}

	ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");

	return ret;
}
