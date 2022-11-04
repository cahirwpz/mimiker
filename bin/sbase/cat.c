/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-u] [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int fd, ret = 0;

	ARGBEGIN {
	case 'u':
		break;
	default:
		usage();
	} ARGEND

	if (!argc) {
		if (concat(0, "<stdin>", 1, "<stdout>") < 0)
			ret = 1;
	} else {
		for (; *argv; argc--, argv++) {
			if (!strcmp(*argv, "-")) {
				*argv = "<stdin>";
				fd = 0;
			} else if ((fd = open(*argv, O_RDONLY)) < 0) {
				weprintf("open %s:", *argv);
				ret = 1;
				continue;
			}
			switch (concat(fd, *argv, 1, "<stdout>")) {
			case -1:
				ret = 1;
				break;
			case -2:
				return 1;  /* exit on write error */
			}
			if (fd != 0)
				close(fd);
		}
	}

	return ret;
}
