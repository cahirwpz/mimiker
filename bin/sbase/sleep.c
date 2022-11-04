/* See LICENSE file for copyright and license details. */
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s num\n", argv0);
}

int
main(int argc, char *argv[])
{
	unsigned seconds;

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (argc != 1)
		usage();

	seconds = estrtonum(argv[0], 0, UINT_MAX);
	while ((seconds = sleep(seconds)) > 0)
		;

	return 0;
}
