/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <unistd.h>

#include "fs.h"
#include "util.h"

static int   hflag = 0;
static gid_t gid = -1;
static int   ret = 0;

static void
chgrp(int dirfd, const char *name, struct stat *st, void *data, struct recursor *r)
{
	int flags = 0;

	if ((r->maxdepth == 0 && r->follow == 'P') || (r->follow == 'H' && r->depth) || (hflag && !(r->depth)))
		flags |= AT_SYMLINK_NOFOLLOW;
	if (fchownat(dirfd, name, -1, gid, flags) < 0) {
		weprintf("chown %s:", r->path);
		ret = 1;
	} else if (S_ISDIR(st->st_mode)) {
		recurse(dirfd, name, NULL, r);
	}
}

static void
usage(void)
{
	eprintf("usage: %s [-h] [-R [-H | -L | -P]] group file ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct group *gr;
	struct recursor r = { .fn = chgrp, .maxdepth = 1, .follow = 'P' };

	ARGBEGIN {
	case 'h':
		hflag = 1;
		break;
	case 'R':
		r.maxdepth = 0;
		break;
	case 'H':
	case 'L':
	case 'P':
		r.follow = ARGC();
		break;
	default:
		usage();
	} ARGEND

	if (argc < 2)
		usage();

	errno = 0;
	if ((gr = getgrnam(argv[0]))) {
		gid = gr->gr_gid;
	} else {
		if (errno)
			eprintf("getgrnam %s:", argv[0]);
		gid = estrtonum(argv[0], 0, UINT_MAX);
	}

	for (argc--, argv++; *argv; argc--, argv++)
		recurse(AT_FDCWD, *argv, NULL, &r);

	return ret || recurse_status;
}
