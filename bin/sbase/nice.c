/* See LICENSE file for copyright and license details. */
#include <sys/resource.h>

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

#ifndef PRIO_MIN
#define PRIO_MIN -NZERO
#endif

#ifndef PRIO_MAX
#define PRIO_MAX (NZERO-1)
#endif

static void
usage(void)
{
	eprintf("usage: %s [-n inc] cmd [arg ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int val = 10, r, savederrno;

	ARGBEGIN {
	case 'n':
		val = estrtonum(EARGF(usage()), PRIO_MIN, PRIO_MAX);
		break;
	default:
		usage();
		break;
	} ARGEND

	if (!argc)
		usage();

	errno = 0;
	r = getpriority(PRIO_PROCESS, 0);
	if (errno)
		weprintf("getpriority:");
	else
		val += r;
	LIMIT(val, PRIO_MIN, PRIO_MAX);
	if (setpriority(PRIO_PROCESS, 0, val) < 0)
		weprintf("setpriority:");

	execvp(argv[0], argv);
	savederrno = errno;
	weprintf("execvp %s:", argv[0]);

	_exit(126 + (savederrno == ENOENT));
}
