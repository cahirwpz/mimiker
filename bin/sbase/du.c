/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "fs.h"
#include "util.h"

static size_t maxdepth = SIZE_MAX;
static size_t blksize  = 512;

static int aflag = 0;
static int sflag = 0;
static int hflag = 0;

static void
printpath(off_t n, const char *path)
{
	if (hflag)
		printf("%s\t%s\n", humansize(n * blksize), path);
	else
		printf("%jd\t%s\n", (intmax_t)n, path);
}

static off_t
nblks(blkcnt_t blocks)
{
	return (512 * blocks + blksize - 1) / blksize;
}

static void
du(int dirfd, const char *path, struct stat *st, void *data, struct recursor *r)
{
	off_t *total = data, subtotal;

	subtotal = nblks(st->st_blocks);
	if (S_ISDIR(st->st_mode))
		recurse(dirfd, path, &subtotal, r);
	*total += subtotal;

	if (!r->depth)
		printpath(*total, r->path);
	else if (!sflag && r->depth <= maxdepth && (S_ISDIR(st->st_mode) || aflag))
		printpath(subtotal, r->path);
}

static void
usage(void)
{
	eprintf("usage: %s [-a | -s] [-d depth] [-h] [-k] [-H | -L | -P] [-x] [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct recursor r = { .fn = du, .follow = 'P' };
	off_t n = 0;
	int kflag = 0, dflag = 0;
	char *bsize;

	ARGBEGIN {
	case 'a':
		aflag = 1;
		break;
	case 'd':
		dflag = 1;
		maxdepth = estrtonum(EARGF(usage()), 0, MIN(LLONG_MAX, SIZE_MAX));
		break;
	case 'h':
		hflag = 1;
		break;
	case 'k':
		kflag = 1;
		break;
	case 's':
		sflag = 1;
		break;
	case 'x':
		r.flags |= SAMEDEV;
		break;
	case 'H':
	case 'L':
	case 'P':
		r.follow = ARGC();
		break;
	default:
		usage();
	} ARGEND

	if ((aflag && sflag) || (dflag && sflag))
		usage();

	bsize = getenv("BLOCKSIZE");
	if (bsize)
		blksize = estrtonum(bsize, 1, MIN(LLONG_MAX, SIZE_MAX));
	if (kflag)
		blksize = 1024;

	if (!argc) {
		recurse(AT_FDCWD, ".", &n, &r);
	} else {
		for (; *argv; argc--, argv++) {
			n = 0;
			recurse(AT_FDCWD, *argv, &n, &r);
		}
	}

	return fshut(stdout, "<stdout>") || recurse_status;
}
