/* See LICENSE file for copyright and license details. */
#include <stdio.h>

#include "../util.h"

void
putword(FILE *fp, const char *s)
{
	static int first = 1;

	if (!first)
		fputc(' ', fp);

	fputs(s, fp);
	first = 0;
}
