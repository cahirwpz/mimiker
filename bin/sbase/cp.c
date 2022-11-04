/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include "fs.h"
#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-afpv] [-R [-H | -L | -P]] source ... dest\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stat st;

	ARGBEGIN {
	case 'a':
		cp_follow = 'P';
		cp_aflag = cp_pflag = cp_rflag = 1;
		break;
	case 'f':
		cp_fflag = 1;
		break;
	case 'p':
		cp_pflag = 1;
		break;
	case 'r':
	case 'R':
		cp_rflag = 1;
		break;
	case 'v':
		cp_vflag = 1;
		break;
	case 'H':
	case 'L':
	case 'P':
		cp_follow = ARGC();
		break;
	default:
		usage();
	} ARGEND

	if (argc < 2)
		usage();

	if (!cp_follow)
		cp_follow = cp_rflag ? 'P' : 'L';

	if (argc > 2) {
		if (stat(argv[argc - 1], &st) < 0)
			eprintf("stat %s:", argv[argc - 1]);
		if (!S_ISDIR(st.st_mode))
			eprintf("%s: not a directory\n", argv[argc - 1]);
	}
	enmasse(argc, argv, cp);

	return fshut(stdout, "<stdout>") || cp_status;
}
