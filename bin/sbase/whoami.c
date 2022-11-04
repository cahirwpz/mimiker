/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s\n", argv0);
}

int
main(int argc, char *argv[])
{
	uid_t uid;
	struct passwd *pw;

	argv0 = *argv, argv0 ? (argc--, argv++) : (void *)0;

	if (argc)
		usage();

	uid = geteuid();
	errno = 0;
	if (!(pw = getpwuid(uid))) {
		if (errno)
			eprintf("getpwuid %d:", uid);
		else
			eprintf("getpwuid %d: no such user\n", uid);
	}
	puts(pw->pw_name);

	return fshut(stdout, "<stdout>");
}
