/* See LICENSE file for copyright and license details. */
#include "../utf.h"

size_t
utftorunestr(const char *str, Rune *r)
{
	size_t i;
	int n;

	for (i = 0; (n = chartorune(&r[i], str)) && r[i]; i++)
		str += n;

	return i;
}

size_t
utfntorunestr(const char *str, size_t len, Rune *r)
{
	size_t i;
	int n;
	const char *end = str + len;

	for (i = 0; (n = charntorune(&r[i], str, end - str)); i++)
		str += n;

	return i;
}
