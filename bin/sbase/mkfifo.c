/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <stdlib.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-m mode] name ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	mode_t mode = 0666;
	int ret = 0;

	ARGBEGIN {
	case 'm':
		mode = parsemode(EARGF(usage()), mode, umask(0));
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		if (mkfifo(*argv, mode) < 0) {
			weprintf("mkfifo %s:", *argv);
			ret = 1;
		}
	}

	return ret;
}
