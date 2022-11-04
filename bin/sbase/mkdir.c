/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <stdlib.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-p] [-m mode] name ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	mode_t mode, mask;
	int pflag = 0, ret = 0;

	mask = umask(0);
	mode = 0777 & ~mask;

	ARGBEGIN {
	case 'p':
		pflag = 1;
		break;
	case 'm':
		mode = parsemode(EARGF(usage()), 0777, mask);
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		if (pflag) {
			if (mkdirp(*argv, mode, 0777 & (~mask | 0300)) < 0)
				ret = 1;
		} else if (mkdir(*argv, mode) < 0) {
			weprintf("mkdir %s:", *argv);
			ret = 1;
		}
	}

	return ret;
}
