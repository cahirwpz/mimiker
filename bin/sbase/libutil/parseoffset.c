/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "../util.h"

off_t
parseoffset(const char *str)
{
	off_t res, scale = 1;
	char *end;

	/* strictly check what strtol() usually would let pass */
	if (!str || !*str || isspace(*str) || *str == '+' || *str == '-') {
		weprintf("parseoffset %s: invalid value\n", str);
		return -1;
	}

	errno = 0;
	res = strtol(str, &end, 0);
	if (errno) {
		weprintf("parseoffset %s: invalid value\n", str);
		return -1;
	}
	if (res < 0) {
		weprintf("parseoffset %s: negative value\n", str);
		return -1;
	}

	/* suffix */
	if (*end) {
		switch (toupper((int)*end)) {
		case 'B':
			scale = 512L;
			break;
		case 'K':
			scale = 1024L;
			break;
		case 'M':
			scale = 1024L * 1024L;
			break;
		case 'G':
			scale = 1024L * 1024L * 1024L;
			break;
		default:
			weprintf("parseoffset %s: invalid suffix '%s'\n", str, end);
			return -1;
		}
	}

	/* prevent overflow */
	if (res > (SSIZE_MAX / scale)) {
		weprintf("parseoffset %s: out of range\n", str);
		return -1;
	}

	return res * scale;
}
