/* See LICENSE file for copyright and license details. */
#include <fcntl.h>
#include <sys/stat.h>

#include "fs.h"
#include "util.h"

static char  *modestr = "";
static mode_t mask    = 0;
static int    ret     = 0;

static void
chmodr(int dirfd, const char *name, struct stat *st, void *data, struct recursor *r)
{
	mode_t m;

	m = parsemode(modestr, st->st_mode, mask);
	if (!S_ISLNK(st->st_mode) && fchmodat(dirfd, name, m, 0) < 0) {
		weprintf("chmod %s:", r->path);
		ret = 1;
	} else if (S_ISDIR(st->st_mode)) {
		recurse(dirfd, name, NULL, r);
	}
}

static void
usage(void)
{
	eprintf("usage: %s [-R] mode file ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct recursor r = { .fn = chmodr, .maxdepth = 1, .follow = 'H', .flags = DIRFIRST };
	size_t i;

	argv0 = *argv, argv0 ? (argc--, argv++) : (void *)0;

	for (; *argv && (*argv)[0] == '-'; argc--, argv++) {
		if (!(*argv)[1])
			usage();
		for (i = 1; (*argv)[i]; i++) {
			switch ((*argv)[i]) {
			case 'R':
				r.maxdepth = 0;
				break;
			case 'r': case 'w': case 'x': case 'X': case 's': case 't':
				/* -[rwxXst] are valid modes, so we're done */
				if (i == 1)
					goto done;
				/* fallthrough */
			case '-':
				/* -- terminator */
				if (i == 1 && !(*argv)[i + 1]) {
					argv++;
					argc--;
					goto done;
				}
				/* fallthrough */
			default:
				usage();
			}
		}
	}
done:
	mask = getumask();
	modestr = *argv;

	if (argc < 2)
		usage();

	for (--argc, ++argv; *argv; argc--, argv++)
		recurse(AT_FDCWD, *argv, NULL, &r);

	return ret || recurse_status;
}
