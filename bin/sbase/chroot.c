/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s dir [cmd [arg ...]]\n", argv0);
}

int
main(int argc, char *argv[])
{
	char *shell[] = { "/bin/sh", "-i", NULL }, *aux, *cmd;
	int savederrno;

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	if ((aux = getenv("SHELL")))
		shell[0] = aux;

	if (chroot(argv[0]) < 0)
		eprintf("chroot %s:", argv[0]);

	if (chdir("/") < 0)
		eprintf("chdir:");

	if (argc == 1) {
		cmd = *shell;
		execvp(cmd, shell);
	} else {
		cmd = argv[1];
		execvp(cmd, argv + 1);
	}

	savederrno = errno;
	weprintf("execvp %s:", cmd);

	_exit(126 + (savederrno == ENOENT));
}
