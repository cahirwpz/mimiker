/* See LICENSE file for copyright and license details. */
#include <sys/file.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-nosux] file cmd [arg ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int fd, status, savederrno, flags = LOCK_EX, nonblk = 0, oflag = 0;
	pid_t pid;

	ARGBEGIN {
	case 'n':
		nonblk = LOCK_NB;
		break;
	case 'o':
		oflag = 1;
		break;
	case 's':
		flags = LOCK_SH;
		break;
	case 'u':
		flags = LOCK_UN;
		break;
	case 'x':
		flags = LOCK_EX;
		break;
	default:
		usage();
	} ARGEND

	if (argc < 2)
		usage();

	if ((fd = open(*argv, O_RDONLY | O_CREAT, 0644)) < 0)
		eprintf("open %s:", *argv);

	if (flock(fd, flags | nonblk)) {
		if (nonblk && errno == EWOULDBLOCK)
			return 1;
		eprintf("flock:");
	}

	switch ((pid = fork())) {
	case -1:
		eprintf("fork:");
	case 0:
		if (oflag && close(fd) < 0)
			eprintf("close:");
		argv++;
		execvp(*argv, argv);
		savederrno = errno;
		weprintf("execvp %s:", *argv);
		_exit(126 + (savederrno == ENOENT));
	default:
		break;
	}
	if (waitpid(pid, &status, 0) < 0)
		eprintf("waitpid:");

	if (close(fd) < 0)
		eprintf("close:");

	if (WIFSIGNALED(status))
		return 128 + WTERMSIG(status);
	if (WIFEXITED(status))
		return WEXITSTATUS(status);

	return 0;
}
