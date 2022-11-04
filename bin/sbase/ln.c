/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-f] [-L | -P | -s] target [name]\n"
	        "       %s [-f] [-L | -P | -s] target ... dir\n", argv0, argv0);
}

int
main(int argc, char *argv[])
{
	char *targetdir = ".", *target = NULL;
	int ret = 0, sflag = 0, fflag = 0, dirfd = AT_FDCWD,
	    hastarget = 0, flags = AT_SYMLINK_FOLLOW;
	struct stat st, tst;

	ARGBEGIN {
	case 'f':
		fflag = 1;
		break;
	case 'L':
		flags |= AT_SYMLINK_FOLLOW;
		break;
	case 'P':
		flags &= ~AT_SYMLINK_FOLLOW;
		break;
	case 's':
		sflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	if (argc > 1) {
		if (!stat(argv[argc - 1], &st) && S_ISDIR(st.st_mode)) {
			if ((dirfd = open(argv[argc - 1], O_RDONLY)) < 0)
				eprintf("open %s:", argv[argc - 1]);
			targetdir = argv[argc - 1];
			if (targetdir[strlen(targetdir) - 1] == '/')
				targetdir[strlen(targetdir) - 1] = '\0';
		} else if (argc == 2) {
			hastarget = 1;
			target = argv[argc - 1];
		} else {
			eprintf("%s: not a directory\n", argv[argc - 1]);
		}
		argv[argc - 1] = NULL;
		argc--;
	}

	for (; *argv; argc--, argv++) {
		if (!hastarget)
			target = basename(*argv);

		if (!sflag) {
			if (stat(*argv, &st) < 0) {
				weprintf("stat %s:", *argv);
				ret = 1;
				continue;
			} else if (fstatat(dirfd, target, &tst, AT_SYMLINK_NOFOLLOW) < 0) {
				if (errno != ENOENT) {
					weprintf("fstatat %s %s:", targetdir, target);
					ret = 1;
					continue;
				}
			} else if (st.st_dev == tst.st_dev && st.st_ino == tst.st_ino) {
				if (!fflag) {
					weprintf("%s and %s/%s are the same file\n",
							*argv, targetdir, target);
					ret = 1;
				}
				continue;
			}
		}

		if (fflag && unlinkat(dirfd, target, 0) < 0 && errno != ENOENT) {
			weprintf("unlinkat %s %s:", targetdir, target);
			ret = 1;
			continue;
		}
		if ((sflag ? symlinkat(*argv, dirfd, target) :
		             linkat(AT_FDCWD, *argv, dirfd, target, flags)) < 0) {
			weprintf("%s %s <- %s/%s:", sflag ? "symlinkat" : "linkat",
			         *argv, targetdir, target);
			ret = 1;
		}
	}

	return ret;
}
