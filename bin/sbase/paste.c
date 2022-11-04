/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <string.h>

#include "utf.h"
#include "util.h"

struct fdescr {
	FILE *fp;
	const char *name;
};

static void
sequential(struct fdescr *dsc, int fdescrlen, Rune *delim, size_t delimlen)
{
	Rune c, last;
	size_t i, d;

	for (i = 0; i < fdescrlen; i++) {
		d = 0;
		last = 0;

		while (efgetrune(&c, dsc[i].fp, dsc[i].name)) {
			if (last == '\n') {
				if (delim[d] != '\0')
					efputrune(&delim[d], stdout, "<stdout>");
				d = (d + 1) % delimlen;
			}

			if (c != '\n')
				efputrune(&c, stdout, "<stdout>");
			last = c;
		}

		if (last == '\n')
			efputrune(&last, stdout, "<stdout>");
	}
}

static void
parallel(struct fdescr *dsc, int fdescrlen, Rune *delim, size_t delimlen)
{
	Rune c, d;
	size_t i, m;
	ssize_t last;

nextline:
	last = -1;

	for (i = 0; i < fdescrlen; i++) {
		d = delim[i % delimlen];
		c = 0;

		while (efgetrune(&c, dsc[i].fp, dsc[i].name)) {
			for (m = last + 1; m < i; m++) {
				if (delim[m % delimlen] != '\0')
					efputrune(&delim[m % delimlen], stdout, "<stdout>");
			}
			last = i;
			if (c == '\n') {
				if (i != fdescrlen - 1)
					c = d;
				efputrune(&c, stdout, "<stdout>");
				break;
			}
			efputrune(&c, stdout, "<stdout>");
		}

		if (c == 0 && last != -1) {
			if (i == fdescrlen - 1)
				putchar('\n');
			else if (d != '\0')
				efputrune(&d, stdout, "<stdout>");
			last++;
		}
	}
	if (last != -1)
		goto nextline;
}

static void
usage(void)
{
	eprintf("usage: %s [-s] [-d list] file ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct fdescr *dsc;
	Rune *delim_rune = NULL;
	size_t delim_runelen, i, delim_bytelen = 1;
	int seq = 0, ret = 0;
	char *delim = "\t";

	ARGBEGIN {
	case 's':
		seq = 1;
		break;
	case 'd':
		delim = EARGF(usage());
		delim_bytelen = unescape(delim);
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	/* populate delimiters */
	delim_rune = ereallocarray(NULL,
		utfmemlen(delim, delim_bytelen) + 1, sizeof(*delim_rune));
	if (!(delim_runelen = utfntorunestr(delim, delim_bytelen, delim_rune))) {
		usage();
	}

	/* populate file list */
	dsc = ereallocarray(NULL, argc, sizeof(*dsc));

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-")) {
			argv[i] = "<stdin>";
			dsc[i].fp = stdin;
		} else if (!(dsc[i].fp = fopen(argv[i], "r"))) {
			eprintf("fopen %s:", argv[i]);
		}
		dsc[i].name = argv[i];
	}

	if (seq) {
		sequential(dsc, argc, delim_rune, delim_runelen);
	} else {
		parallel(dsc, argc, delim_rune, delim_runelen);
	}

	for (i = 0; i < argc; i++)
		if (dsc[i].fp != stdin && fshut(dsc[i].fp, argv[i]))
			ret |= fshut(dsc[i].fp, argv[i]);

	ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");

	return ret;
}
