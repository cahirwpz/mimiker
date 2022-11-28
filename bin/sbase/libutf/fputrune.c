/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utf.h"

int
fputrune(const Rune *r, FILE *fp)
{
	char buf[UTFmax];

	return fwrite(buf, runetochar(buf, r), 1, fp);
}

int
efputrune(const Rune *r, FILE *fp, const char *file)
{
	int ret;

	if ((ret = fputrune(r, fp)) < 0) {
		fprintf(stderr, "fputrune %s: %s\n", file, strerror(errno));
		exit(1);
	}
	return ret;
}
