/* See LICENSE file for copyright and license details. */
#include <stdlib.h>
#include <string.h>

#include "../util.h"

void *
ecalloc(size_t nmemb, size_t size)
{
	return encalloc(1, nmemb, size);
}

void *
emalloc(size_t size)
{
	return enmalloc(1, size);
}

void *
erealloc(void *p, size_t size)
{
	return enrealloc(1, p, size);
}

char *
estrdup(const char *s)
{
	return enstrdup(1, s);
}

char *
estrndup(const char *s, size_t n)
{
	return enstrndup(1, s, n);
}

void *
encalloc(int status, size_t nmemb, size_t size)
{
	void *p;

	p = calloc(nmemb, size);
	if (!p)
		enprintf(status, "calloc: out of memory\n");
	return p;
}

void *
enmalloc(int status, size_t size)
{
	void *p;

	p = malloc(size);
	if (!p)
		enprintf(status, "malloc: out of memory\n");
	return p;
}

void *
enrealloc(int status, void *p, size_t size)
{
	p = realloc(p, size);
	if (!p)
		enprintf(status, "realloc: out of memory\n");
	return p;
}

char *
enstrdup(int status, const char *s)
{
	char *p;

	p = strdup(s);
	if (!p)
		enprintf(status, "strdup: out of memory\n");
	return p;
}

char *
enstrndup(int status, const char *s, size_t n)
{
	char *p;

	p = strndup(s, n);
	if (!p)
		enprintf(status, "strndup: out of memory\n");
	return p;
}
