/* See LICENSE file for copyright and license details. */
#include <sys/resource.h>

#include <errno.h>
#include <pwd.h>
#include <stdlib.h>

#include "util.h"

#ifndef PRIO_MIN
#define PRIO_MIN -NZERO
#endif

#ifndef PRIO_MAX
#define PRIO_MAX (NZERO-1)
#endif

static int
renice(int which, int who, long adj)
{
	errno = 0;
	adj += getpriority(which, who);
	if (errno) {
		weprintf("getpriority %d:", who);
		return 0;
	}

	adj = MAX(PRIO_MIN, MIN(adj, PRIO_MAX));
	if (setpriority(which, who, (int)adj) < 0) {
		weprintf("setpriority %d:", who);
		return 0;
	}

	return 1;
}

static void
usage(void)
{
	eprintf("usage: %s -n num [-g | -p | -u] id ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	const char *adj = NULL;
	long val;
	int which = PRIO_PROCESS, ret = 0;
	struct passwd *pw;
	int who;

	ARGBEGIN {
	case 'n':
		adj = EARGF(usage());
		break;
	case 'g':
		which = PRIO_PGRP;
		break;
	case 'p':
		which = PRIO_PROCESS;
		break;
	case 'u':
		which = PRIO_USER;
		break;
	default:
		usage();
	} ARGEND

	if (!argc || !adj)
		usage();

	val = estrtonum(adj, PRIO_MIN, PRIO_MAX);
	for (; *argv; argc--, argv++) {
		if (which == PRIO_USER) {
			errno = 0;
			if (!(pw = getpwnam(*argv))) {
				if (errno)
					weprintf("getpwnam %s:", *argv);
				else
					weprintf("getpwnam %s: no user found\n", *argv);
				ret = 1;
				continue;
			}
			who = pw->pw_uid;
		} else {
			who = estrtonum(*argv, 1, INT_MAX);
		}
		if (!renice(which, who, val))
			ret = 1;
	}

	return ret;
}
