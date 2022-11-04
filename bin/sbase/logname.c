/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s\n", argv0);
}

int
main(int argc, char *argv[])
{
	char *login;

	argv0 = *argv, argv0 ? (argc--, argv++) : (void *)0;

	if (argc)
		usage();

	if ((login = getlogin()))
		puts(login);
	else
		eprintf("no login name\n");

	return fshut(stdout, "<stdout>");
}
