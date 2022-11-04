/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

static void
usage(void)
{
	enprintf(2, "usage: %s [-l | -s] file1 file2\n", argv0);
}

int
main(int argc, char *argv[])
{
	FILE *fp[2];
	size_t line = 1, n;
	int ret = 0, lflag = 0, sflag = 0, same = 1, b[2];

	ARGBEGIN {
	case 'l':
		lflag = 1;
		break;
	case 's':
		sflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if (argc != 2 || (lflag && sflag))
		usage();

	for (n = 0; n < 2; n++) {
		if (!strcmp(argv[n], "-")) {
			argv[n] = "<stdin>";
			fp[n] = stdin;
		} else {
			if (!(fp[n] = fopen(argv[n], "r"))) {
				if (!sflag)
					weprintf("fopen %s:", argv[n]);
				return 2;
			}
		}
	}

	for (n = 1; ; n++) {
		b[0] = getc(fp[0]);
		b[1] = getc(fp[1]);

		if (b[0] == b[1]) {
			if (b[0] == EOF)
				break;
			else if (b[0] == '\n')
				line++;
			continue;
		} else if (b[0] == EOF || b[1] == EOF) {
			if (!sflag)
				weprintf("cmp: EOF on %s\n", argv[(b[0] != EOF)]);
			same = 0;
			break;
		} else if (!lflag) {
			if (!sflag)
				printf("%s %s differ: byte %zu, line %zu\n",
				       argv[0], argv[1], n, line);
			same = 0;
			break;
		} else {
			printf("%zu %o %o\n", n, b[0], b[1]);
			same = 0;
		}
	}

	if (!ret)
		ret = !same;
	if (fshut(fp[0], argv[0]) | (fp[0] != fp[1] && fshut(fp[1], argv[1])) |
	    fshut(stdout, "<stdout>"))
		ret = 2;

	return ret;
}
