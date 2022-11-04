/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utf.h"

int
fgetrune(Rune *r, FILE *fp)
{
	char buf[UTFmax];
	int  i = 0, c;

	while (i < UTFmax && (c = fgetc(fp)) != EOF) {
		buf[i++] = c;
		if (charntorune(r, buf, i) > 0)
			break;
	}
	if (ferror(fp))
		return -1;

	return i;
}

int
efgetrune(Rune *r, FILE *fp, const char *file)
{
	int ret;

	if ((ret = fgetrune(r, fp)) < 0) {
		fprintf(stderr, "fgetrune %s: %s\n", file, strerror(errno));
		exit(1);
	}
	return ret;
}
