/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "utf.h"
#include "util.h"

static int     iflag      = 0;
static size_t *tablist    = NULL;
static size_t  tablistlen = 0;

static size_t
parselist(const char *s)
{
	size_t i;
	char  *p, *tmp;

	tmp = estrdup(s);
	for (i = 0; (p = strsep(&tmp, " ,")); i++) {
		if (*p == '\0')
			eprintf("empty field in tablist\n");
		tablist = ereallocarray(tablist, i + 1, sizeof(*tablist));
		tablist[i] = estrtonum(p, 1, MIN(LLONG_MAX, SIZE_MAX));
		if (i > 0 && tablist[i - 1] >= tablist[i])
			eprintf("tablist must be ascending\n");
	}
	tablist = ereallocarray(tablist, i + 1, sizeof(*tablist));
	/* tab length = 1 for the overflowing case later in the matcher */
	tablist[i] = 1;

	return i;
}

static int
expand(const char *file, FILE *fp)
{
	size_t bol = 1, col = 0, i;
	Rune r;

	while (efgetrune(&r, fp, file)) {
		switch (r) {
		case '\t':
			if (tablistlen == 1)
				i = 0;
			else for (i = 0; i < tablistlen; i++)
				if (col < tablist[i])
					break;
			if (bol || !iflag) {
				do {
					col++;
					putchar(' ');
				} while (col % tablist[i]);
			} else {
				putchar('\t');
				col = tablist[i];
			}
			break;
		case '\b':
			bol = 0;
			if (col)
				col--;
			putchar('\b');
			break;
		case '\n':
			bol = 1;
			col = 0;
			putchar('\n');
			break;
		default:
			col++;
			if (r != ' ')
				bol = 0;
			efputrune(&r, stdout, "<stdout>");
			break;
		}
	}

	return 0;
}

static void
usage(void)
{
	eprintf("usage: %s [-i] [-t tablist] [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	int ret = 0;
	char *tl = "8";

	ARGBEGIN {
	case 'i':
		iflag = 1;
		break;
	case 't':
		tl = EARGF(usage());
		if (!*tl)
			eprintf("tablist cannot be empty\n");
		break;
	default:
		usage();
	} ARGEND

	tablistlen = parselist(tl);

	if (!argc) {
		expand("<stdin>", stdin);
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
			expand(*argv, fp);
			if (fp != stdin && fshut(fp, *argv))
				ret = 1;
		}
	}

	ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");

	return ret;
}
