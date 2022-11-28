/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../util.h"

char *
humansize(off_t n)
{
	static char buf[16];
	const char postfixes[] = "BKMGTPE";
	double size;
	int i;

	for (size = n, i = 0; size >= 1024 && i < (int)strlen(postfixes); i++)
		size /= 1024;

	if (!i)
		snprintf(buf, sizeof(buf), "%ju", (uintmax_t)n);
	else
		snprintf(buf, sizeof(buf), "%.1f%c", size, postfixes[i]);

	return buf;
}
