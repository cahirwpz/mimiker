/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>
#include <sys/types.h>
#ifndef makedev
#include <sys/sysmacros.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-m mode] name b|c|u major minor\n"
	        "       %s [-m mode] name p\n",
	        argv0, argv0);
}

int
main(int argc, char *argv[])
{
	mode_t mode = 0666;
	dev_t dev;

	ARGBEGIN {
	case 'm':
		mode = parsemode(EARGF(usage()), mode, umask(0));
		break;
	default:
		usage();
	} ARGEND;

	if (argc < 2)
		usage();

	if (strlen(argv[1]) != 1)
		goto invalid;
	switch (argv[1][0]) {
	case 'b':
		mode |= S_IFBLK;
		break;
	case 'u':
	case 'c':
		mode |= S_IFCHR;
		break;
	case 'p':
		mode |= S_IFIFO;
		break;
	default:
	invalid:
		eprintf("invalid type '%s'\n", argv[1]);
	}

	if (S_ISFIFO(mode)) {
		if (argc != 2)
			usage();
		dev = 0;
	} else {
		if (argc != 4)
			usage();
		dev = makedev(estrtonum(argv[2], 0, LLONG_MAX), estrtonum(argv[3], 0, LLONG_MAX));
	}

	if (mknod(argv[0], mode, dev) == -1)
		eprintf("mknod %s:", argv[0]);
	return 0;
}
