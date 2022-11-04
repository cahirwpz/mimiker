/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "text.h"
#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [file ...]\n", argv0);
}

static void
rev(FILE *fp)
{
	static char *line = NULL;
	static size_t size = 0;
	size_t i;
	ssize_t n;
	int lf;

	while ((n = getline(&line, &size, fp)) > 0) {
		lf = n && line[n - 1] == '\n';
		i = n -= lf;
		for (n = 0; i--;) {
			if ((line[i] & 0xC0) == 0x80) {
				n++;
			} else {
				fwrite(line + i, 1, n + 1, stdout);
				n = 0;
			}
		}
		if (n)
			fwrite(line, 1, n, stdout);
		if (lf)
			fputc('\n', stdout);
	}
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	int ret = 0;

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (!argc) {
		rev(stdin);
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
			rev(fp);
			if (fp != stdin && fshut(fp, *argv))
				ret = 1;
		}
	}

	ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");

	return ret;
}
