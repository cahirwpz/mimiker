/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"

static const char *
getpwd(const char *cwd)
{
	const char *pwd;
	struct stat cst, pst;

	if (!(pwd = getenv("PWD")) || pwd[0] != '/' || stat(pwd, &pst) < 0)
		return cwd;
	if (stat(cwd, &cst) < 0)
		eprintf("stat %s:", cwd);

	return (pst.st_dev == cst.st_dev && pst.st_ino == cst.st_ino) ? pwd : cwd;
}

static void
usage(void)
{
	eprintf("usage: %s [-LP]\n", argv0);
}

int
main(int argc, char *argv[])
{
	char cwd[PATH_MAX];
	char mode = 'L';

	ARGBEGIN {
	case 'L':
	case 'P':
		mode = ARGC();
		break;
	default:
		usage();
	} ARGEND

	if (!getcwd(cwd, sizeof(cwd)))
		eprintf("getcwd:");
	puts((mode == 'L') ? getpwd(cwd) : cwd);

	return fshut(stdout, "<stdout>");
}
