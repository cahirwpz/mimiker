/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "fs.h"
#include "util.h"

static int mv_status = 0;

static int
mv(const char *s1, const char *s2, int depth)
{
	struct recursor r = { .fn = rm, .follow = 'P' };

	if (!rename(s1, s2))
		return 0;
	if (errno == EXDEV) {
		cp_aflag = cp_rflag = cp_pflag = 1;
		cp_follow = 'P';
		cp_status = 0;
		rm_status = 0;
		cp(s1, s2, depth);
		if (cp_status == 0)
			recurse(AT_FDCWD, s1, NULL, &r);
		if (cp_status || rm_status)
			mv_status = 1;
	} else {
		weprintf("%s -> %s:", s1, s2);
		mv_status = 1;
	}

	return 0;
}

static void
usage(void)
{
	eprintf("usage: %s [-f] source ... dest\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stat st;

	ARGBEGIN {
	case 'f':
		break;
	default:
		usage();
	} ARGEND

	if (argc < 2)
		usage();

	if (argc > 2) {
		if (stat(argv[argc - 1], &st) < 0)
			eprintf("stat %s:", argv[argc - 1]);
		if (!S_ISDIR(st.st_mode))
			eprintf("%s: not a directory\n", argv[argc - 1]);
	}
	enmasse(argc, argv, mv);

	return mv_status;
}
