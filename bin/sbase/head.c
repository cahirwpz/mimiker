/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

static void
head(FILE *fp, const char *fname, size_t n)
{
	char *buf = NULL;
	size_t i = 0, size = 0;
	ssize_t len;

	while (i < n && (len = getline(&buf, &size, fp)) > 0) {
		fwrite(buf, 1, len, stdout);
		i += (len && (buf[len - 1] == '\n'));
	}
	free(buf);
	if (ferror(fp))
		eprintf("getline %s:", fname);
}

static void
usage(void)
{
	eprintf("usage: %s [-num | -n num] [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	size_t n = 10;
	FILE *fp;
	int ret = 0, newline = 0, many = 0;

	ARGBEGIN {
	case 'n':
		n = estrtonum(EARGF(usage()), 0, MIN(LLONG_MAX, SIZE_MAX));
		break;
	ARGNUM:
		n = ARGNUMF();
		break;
	default:
		usage();
	} ARGEND

	if (!argc) {
		head(stdin, "<stdin>", n);
	} else {
		many = argc > 1;
		for (newline = 0; *argv; argc--, argv++) {
			if (!strcmp(*argv, "-")) {
				*argv = "<stdin>";
				fp = stdin;
			} else if (!(fp = fopen(*argv, "r"))) {
				weprintf("fopen %s:", *argv);
				ret = 1;
				continue;
			}
			if (many) {
				if (newline)
					putchar('\n');
				printf("==> %s <==\n", *argv);
			}
			newline = 1;
			head(fp, *argv, n);
			if (fp != stdin && fshut(fp, *argv))
				ret = 1;
		}
	}

	ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");

	return ret;
}
