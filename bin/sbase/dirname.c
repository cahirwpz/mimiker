/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <stdio.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s path\n", argv0);
}

int
main(int argc, char *argv[])
{
	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (argc != 1)
		usage();

	puts(dirname(argv[0]));

	return fshut(stdout, "<stdout>");
}
