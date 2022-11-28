/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "../fs.h"
#include "../util.h"

int rm_status = 0;

void
rm(int dirfd, const char *name, struct stat *st, void *data, struct recursor *r)
{
	if (!r->maxdepth && S_ISDIR(st->st_mode)) {
		recurse(dirfd, name, NULL, r);

		if (unlinkat(dirfd, name, AT_REMOVEDIR) < 0) {
			if (!(r->flags & SILENT))
				weprintf("rmdir %s:", r->path);
			if (!((r->flags & SILENT) && errno == ENOENT))
				rm_status = 1;
		}
	} else if (unlinkat(dirfd, name, 0) < 0) {
		if (!(r->flags & SILENT))
			weprintf("unlink %s:", r->path);
		if (!((r->flags & SILENT) && errno == ENOENT))
			rm_status = 1;
	}
}
