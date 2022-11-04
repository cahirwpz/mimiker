/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "../util.h"

double
estrtod(const char *s)
{
	char *end;
	double d;

	d = strtod(s, &end);
	if (end == s || *end != '\0')
		eprintf("%s: not a real number\n", s);
	return d;
}
