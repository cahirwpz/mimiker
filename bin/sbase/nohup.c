/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s cmd [arg ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int fd, savederrno;

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
		enprintf(127, "signal HUP:");

	if (isatty(STDOUT_FILENO)) {
		if ((fd = open("nohup.out", O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
			enprintf(127, "open nohup.out:");
		if (dup2(fd, STDOUT_FILENO) < 0)
			enprintf(127, "dup2:");
		close(fd);
	}
	if (isatty(STDERR_FILENO) && dup2(STDOUT_FILENO, STDERR_FILENO) < 0)
		enprintf(127, "dup2:");

	execvp(argv[0], argv);
	savederrno = errno;
	weprintf("execvp %s:", argv[0]);

	_exit(126 + (savederrno == ENOENT));
}
