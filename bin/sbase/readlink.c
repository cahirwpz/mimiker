/* See LICENSE file for copyright and license details. */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-fn] path\n", argv0);
}

int
main(int argc, char *argv[])
{
	char buf[PATH_MAX];
	ssize_t n;
	int nflag = 0, fflag = 0;

	ARGBEGIN {
	case 'f':
		fflag = ARGC();
		break;
	case 'n':
		nflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if (argc != 1)
		usage();

	if (strlen(argv[0]) >= PATH_MAX)
		eprintf("path too long\n");

	if (fflag) {
		if (!realpath(argv[0], buf))
			eprintf("realpath %s:", argv[0]);
	} else {
		if ((n = readlink(argv[0], buf, PATH_MAX - 1)) < 0)
			eprintf("readlink %s:", argv[0]);
		buf[n] = '\0';
	}

	fputs(buf, stdout);
	if (!nflag)
		putchar('\n');

	return fshut(stdout, "<stdout>");
}
