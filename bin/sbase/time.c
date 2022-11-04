/* See LICENSE file for copyright and license details. */
#include <sys/times.h>
#include <sys/wait.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-p] cmd [arg ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	pid_t pid;
	struct tms tms; /* user and sys times */
	clock_t r0, r1; /* real time */
	long ticks;     /* per second */
	int status, savederrno, ret = 0;

	ARGBEGIN {
	case 'p':
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	if ((ticks = sysconf(_SC_CLK_TCK)) <= 0)
		eprintf("sysconf _SC_CLK_TCK:");

	if ((r0 = times(&tms)) == (clock_t)-1)
		eprintf("times:");

	switch ((pid = fork())) {
	case -1:
		eprintf("fork:");
	case 0:
		execvp(argv[0], argv);
		savederrno = errno;
		weprintf("execvp %s:", argv[0]);
		_exit(126 + (savederrno == ENOENT));
	default:
		break;
	}
	waitpid(pid, &status, 0);

	if ((r1 = times(&tms)) == (clock_t)-1)
		eprintf("times:");

	if (WIFSIGNALED(status)) {
		fprintf(stderr, "Command terminated by signal %d\n",
		        WTERMSIG(status));
		ret = 128 + WTERMSIG(status);
	}

	fprintf(stderr, "real %f\nuser %f\nsys %f\n",
	        (r1 - r0)      / (double)ticks,
	        tms.tms_cutime / (double)ticks,
	        tms.tms_cstime / (double)ticks);

	if (WIFEXITED(status))
		ret = WEXITSTATUS(status);

	return ret;
}
