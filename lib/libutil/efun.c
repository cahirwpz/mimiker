/*	$NetBSD: efun.c,v 1.10 2015/07/26 02:20:30 kamil Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <util.h>

typedef void (*efunc_t)(int, const char *, ...);

static efunc_t efunc = err;

/*
 * GCC doesn't let us cast `exit` to type `efunc_t`
 * (even though Clang does), so we create a wrapper here.
 */
static void efunc_exit(int n, const char *f, ...) {
    exit(n);
}

efunc_t esetfunc(efunc_t ef)
{
	efunc_t of = efunc;
	efunc = ef == NULL ? efunc_exit : ef;
	return of;
}

size_t
estrlcpy(char *dst, const char *src, size_t len)
{
	size_t rv;
	if ((rv = strlcpy(dst, src, len)) >= len) {
		errno = ENAMETOOLONG;
		(*efunc)(1,
		    "Cannot copy string; %zu chars needed %zu provided",
		    rv, len);
	}
	return rv;
}

size_t
estrlcat(char *dst, const char *src, size_t len)
{
	size_t rv;
	if ((rv = strlcat(dst, src, len)) >= len) {
		errno = ENAMETOOLONG;
		(*efunc)(1,
		    "Cannot append to string; %zu chars needed %zu provided",
		    rv, len);
	}
	return rv;
}

char *
estrdup(const char *s)
{
	char *d = strdup(s);
	if (d == NULL)
		(*efunc)(1, "Cannot copy string");
	return d;
}

char *
estrndup(const char *s, size_t len)
{
	char *d = strndup(s, len);
	if (d == NULL)
		(*efunc)(1, "Cannot copy string");
	return d;
}

void *
emalloc(size_t n)
{
	void *p = malloc(n);
	if (p == NULL && n != 0)
		(*efunc)(1, "Cannot allocate %zu bytes", n);
	return p;
}

void *
ecalloc(size_t n, size_t s)
{
	void *p = calloc(n, s);
	if (p == NULL && n != 0 && s != 0)
		(*efunc)(1, "Cannot allocate %zu blocks of size %zu", n, s);
	return p;
}

void *
erealloc(void *p, size_t n)
{
	void *q = realloc(p, n);
	if (q == NULL && n != 0)
		(*efunc)(1, "Cannot re-allocate %zu bytes", n);
	return q;
}

void
ereallocarr(void *p, size_t n, size_t s)
{
	int rv = reallocarr(p, n, s);
	if (rv != 0) {
		errno = rv;
		(*efunc)(1, "Cannot re-allocate %zu * %zu bytes", n, s);
	}
}

FILE *
efopen(const char *p, const char *m)
{
	FILE *fp = fopen(p, m);
	if (fp == NULL)
		(*efunc)(1, "Cannot open `%s'", p);
	return fp;
}

int
easprintf(char ** __restrict ret, const char * __restrict format, ...)
{
	int rv;
	va_list ap;
	va_start(ap, format);
	if ((rv = vasprintf(ret, format, ap)) == -1)
		(*efunc)(1, "Cannot format string");
	va_end(ap);
	return rv;
}

int
evasprintf(char ** __restrict ret, const char * __restrict format, va_list ap)
{
	int rv;
	if ((rv = vasprintf(ret, format, ap)) == -1)
		(*efunc)(1, "Cannot format string");
	return rv;
}

intmax_t
estrtoi(const char * nptr, int base, intmax_t lo, intmax_t hi)
{
	int e;
	intmax_t rv = strtoi(nptr, NULL, base, lo, hi, &e);
	if (e != 0) {
		errno = e;
		(*efunc)(1,
		    "Cannot convert string value '%s' with base %d to a number in range [%jd .. %jd]",
		    nptr, base, lo, hi);
	}
	return rv;
}

uintmax_t
estrtou(const char * nptr, int base, uintmax_t lo, uintmax_t hi)
{
	int e;
	uintmax_t rv = strtou(nptr, NULL, base, lo, hi, &e);
	if (e != 0) {
		errno = e;
		(*efunc)(1,
		    "Cannot convert string value '%s' with base %d to a number in range [%ju .. %ju]",
		    nptr, base, lo, hi);
	}
	return rv;
}
