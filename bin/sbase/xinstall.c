/* See LICENSE file for copyright and license details. */
#include <grp.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "util.h"

static int Dflag = 0;
static gid_t group;
static uid_t owner;
static mode_t mode = 0755;

static void
make_dir(char *dir, int was_missing)
{
	if (!mkdir(dir, was_missing ? 0755 : mode)) {
		if (!was_missing && (lchown(dir, owner, group) < 0))
			eprintf("lchmod %s:", dir);
	} else if (errno != EEXIST) {
		eprintf("mkdir %s:", dir);
	}
}

static void
make_dirs(char *dir, int was_missing)
{
	char *p;
	for (p = strchr(dir + (dir[0] == '/'), '/'); p; p = strchr(p + 1, '/')) {
		*p = '\0';
		make_dir(dir, was_missing);
		*p = '/';
	}
	make_dir(dir, was_missing);
}

static int
install(const char *s1, const char *s2, int depth)
{
	int f1, f2;

	if ((f1 = open(s1, O_RDONLY)) < 0)
		eprintf("open %s:", s1);
	if ((f2 = creat(s2, 0600)) < 0) {
		if (unlink(s2) < 0 && errno != ENOENT)
			eprintf("unlink %s:", s2);
		if ((f2 = creat(s2, 0600)) < 0)
			eprintf("creat %s:", s2);
	}
	if (concat(f1, s1, f2, s2) < 0)
		goto fail;
	if (fchmod(f2, mode) < 0) {
		weprintf("fchmod %s:", s2);
		goto fail;
	}
	if (fchown(f2, owner, group) < 0) {
		weprintf("fchown %s:", s2);
		goto fail;
	}

	close(f1);
	close(f2);

	return 0;

fail:
	unlink(s2);
	exit(1);
}

static void
usage(void)
{
	eprintf("usage: %s [-g group] [-o owner] [-m mode] (-d dir ... | [-D] (-t dest source ... | source ... dest))\n", argv0);
}

int
main(int argc, char *argv[])
{
	int dflag = 0;
	char *gflag = 0;
	char *oflag = 0;
	char *mflag = 0;
	char *tflag = 0;
	struct group *gr;
	struct passwd *pw;
	struct stat st;
	char *p;

	ARGBEGIN {
	case 'c':
		/* no-op for compatibility */
		break;
	case 'd':
		dflag = 1;
		break;
	case 'D':
		Dflag = 1;
		break;
	case 's':
		/* no-op for compatibility */
		break;
	case 'g':
		gflag = EARGF(usage());
		break;
	case 'o':
		oflag = EARGF(usage());
		break;
	case 'm':
		mflag = EARGF(usage());
		break;
	case 't':
		tflag = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	if (argc < 1 + (!tflag & !dflag) || dflag & (Dflag | !!tflag))
		usage();

	if (gflag) {
		errno = 0;
		gr = getgrnam(gflag);
		if (gr) {
			group = gr->gr_gid;
		} else {
			if (errno)
				eprintf("getgrnam %s:", gflag);
			group = estrtonum(gflag, 0, UINT_MAX);
		}
	} else {
		group = getgid();
	}

	if (oflag) {
		errno = 0;
		pw = getpwnam(oflag);
		if (pw) {
			owner = pw->pw_uid;
		} else {
			if (errno)
				eprintf("getpwnam %s:", oflag);
			owner = estrtonum(oflag, 0, UINT_MAX);
		}
	} else {
		owner = getuid();
	}

	if (mflag)
		mode = parsemode(mflag, mode, 0);

	if (dflag) {
		for (; *argv; argc--, argv++)
			make_dirs(*argv, 0);
		return 0;
	}

	if (tflag) {
		argv = memmove(argv - 1, argv, argc * sizeof(*argv));
		argv[argc++] = tflag;
	}
	if (tflag || argc > 2) {
		if (stat(argv[argc - 1], &st) < 0) {
			if ((errno == ENOENT) && Dflag) {
				make_dirs(argv[argc - 1], 1);
			} else {
				eprintf("stat %s:", argv[argc - 1]);
			}
		} else if (!S_ISDIR(st.st_mode)) {
			eprintf("%s: not a directory\n", argv[argc - 1]);
		}
	}
	if (stat(argv[argc - 1], &st) < 0) {
		if (errno != ENOENT)
			eprintf("stat %s:", argv[argc - 1]);
		if (tflag || Dflag || argc > 2) {
			if ((p = strrchr(argv[argc - 1], '/')) != NULL) {
				*p = '\0';
				make_dirs(argv[argc - 1], 1);
				*p = '/';
			}
		}
	}
	enmasse(argc, argv, install);

	return 0;
}
