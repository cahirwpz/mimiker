/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define PORTABLE_CHARACTER_SET "0123456789._-qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM"
/* If your system supports more other characters, but not all non-NUL characters, define SYSTEM_CHARACTER_SET. */

static int most = 0;
static int extra = 0;

static int
pathchk(char *filename)
{
	char *invalid, *invalid_end, *p, *q;
	const char *character_set;
	size_t len, maxlen;
	struct stat st;

	/* Empty? */
	if (extra && !*filename)
		eprintf("empty filename\n");

	/* Leading hyphen? */
	if (extra && ((*filename == '-') || strstr(filename, "/-")))
		eprintf("%s: leading '-' in component of filename\n", filename);

	/* Nonportable character? */
#ifdef SYSTEM_CHARACTER_SET
	character_set = "/"SYSTEM_CHARACTER_SET;
#else
	character_set = 0;
#endif
	if (most)
		character_set = "/"PORTABLE_CHARACTER_SET;
	if (character_set && *(invalid = filename + strspn(filename, character_set))) {
		for (invalid_end = invalid + 1; *invalid_end & 0x80; invalid_end++);
		p = estrdup(filename);
		*invalid_end = 0;
		eprintf("%s: nonportable character '%s'\n", p, invalid);
	}

	/* Symlink error? Non-searchable directory? */
	if (lstat(filename, &st) && errno != ENOENT) {
		/* lstat rather than stat, so that if filename is a bad symlink, but
		 * all parents are OK, no error will be detected. */
		eprintf("%s:", filename);
	}

	/* Too long pathname? */
	maxlen = most ? _POSIX_PATH_MAX : PATH_MAX;
	if (strlen(filename) >= maxlen)
		eprintf("%s: is longer than %zu bytes\n", filename, maxlen);

	/* Too long component? */
	maxlen = most ? _POSIX_NAME_MAX : NAME_MAX;
	for (p = filename; p; p = q) {
		q = strchr(p, '/');
		len = q ? (size_t)(q++ - p) : strlen(p);
		if (len > maxlen)
			eprintf("%s: includes component longer than %zu bytes\n",
			         filename, maxlen);
	}

	return 0;
}

static void
usage(void)
{
	eprintf("usage: %s [-pP] filename...\n", argv0);
}

int
main(int argc, char *argv[])
{
	int ret = 0;

	ARGBEGIN {
	case 'p':
		most = 1;
		break;
	case 'P':
		extra = 1;
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();

	for (; argc--; argv++)
		ret |= pathchk(*argv);

	return ret;
}
