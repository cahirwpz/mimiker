/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static int aflag;

static int
canexec(int fd, const char *name)
{
	struct stat st;

	if (fstatat(fd, name, &st, 0) < 0 || !S_ISREG(st.st_mode))
		return 0;
	return faccessat(fd, name, X_OK, AT_EACCESS) == 0;
}

static int
which(const char *path, const char *name)
{
	char *ptr, *p;
	size_t i, len;
	int dirfd, found = 0;

	if (strchr(name, '/')) {
		found = canexec(AT_FDCWD, name);
		if (found)
			puts(name);
		return found;
	}

	ptr = p = enstrdup(3, path);
	len = strlen(p);
	for (i = 0; i < len + 1; i++) {
		if (ptr[i] != ':' && ptr[i] != '\0')
			continue;
		ptr[i] = '\0';
		if ((dirfd = open(p, O_RDONLY)) >= 0) {
			if (canexec(dirfd, name)) {
				found = 1;
				fputs(p, stdout);
				if (i && ptr[i - 1] != '/')
					fputc('/', stdout);
				puts(name);
			}
			close(dirfd);
			if (!aflag && found)
				break;
		}
		p = ptr + i + 1;
	}
	free(ptr);

	return found;
}

static void
usage(void)
{
	eprintf("usage: %s [-a] name ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	char *path;
	int found = 0, foundall = 1;

	ARGBEGIN {
	case 'a':
		aflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	if (!(path = getenv("PATH")))
		enprintf(3, "$PATH is not set\n");

	for (; *argv; argc--, argv++) {
		if (which(path, *argv)) {
			found = 1;
		} else {
			weprintf("%s: not an external command\n", *argv);
			foundall = 0;
		}
	}

	return found ? foundall ? 0 : 1 : 2;
}
