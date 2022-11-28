/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../util.h"

void
enmasse(int argc, char *argv[], int (*fn)(const char *, const char *, int))
{
	struct stat st;
	char buf[PATH_MAX], *dir;
	int i, len;
	size_t dlen;

	if (argc == 2 && !(stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode))) {
		fnck(argv[0], argv[1], fn, 0);
		return;
	} else {
		dir = (argc == 1) ? "." : argv[--argc];
	}

	for (i = 0; i < argc; i++) {
		dlen = strlen(dir);
		if (dlen > 0 && dir[dlen - 1] == '/')
			len = snprintf(buf, sizeof(buf), "%s%s", dir, basename(argv[i]));
		else
			len = snprintf(buf, sizeof(buf), "%s/%s", dir, basename(argv[i]));
		if (len < 0 || len >= (int)sizeof(buf)) {
			eprintf("%s/%s: filename too long\n", dir,
			        basename(argv[i]));
		}
		fnck(argv[i], buf, fn, 0);
	}
}
