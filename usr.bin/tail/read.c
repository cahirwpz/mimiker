/*	$NetBSD: read.c,v 1.17 2011/09/03 10:59:10 christos Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Sze-Tyan Wang.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "extern.h"

/*
 * displaybytes -- read bytes to an offset from the end and display.
 *
 * This is the function that reads to a byte offset from the end of the input,
 * storing the data in a wrap-around buffer which is then displayed.  If the
 * rflag is set, the data is displayed in lines in reverse order, and this
 * routine has the usual nastiness of trying to find the newlines.  Otherwise,
 * it is displayed from the character closest to the beginning of the input to
 * the end.
 *
 * Non-zero return means than a (non-fatal) error occurred.
 */
int
displaybytes(FILE *fp, off_t off)
{
	int ch, len, tlen;
	char *ep, *p, *t;
	int wrap;
	char *sp;

	if ((sp = p = malloc(off)) == NULL)
		xerr(1, "malloc");

	for (wrap = 0, ep = p + off; (ch = getc(fp)) != EOF;) {
		*p = ch;
		if (++p == ep) {
			wrap = 1;
			p = sp;
		}
	}
	if (ferror(fp)) {
		ierr();
		return 1;
	}

	if (rflag) {
		for (t = p - 1, len = 0; t >= sp; --t, ++len)
			if (*t == '\n' && len) {
				WR(t + 1, len);
				len = 0;
		}
		if (wrap) {
			tlen = len;
			for (t = ep - 1, len = 0; t >= p; --t, ++len)
				if (*t == '\n') {
					if (len) {
						WR(t + 1, len);
						len = 0;
					}
					if (tlen) {
						WR(sp, tlen);
						tlen = 0;
					}
				}
			if (len)
				WR(t + 1, len);
			if (tlen)
				WR(sp, tlen);
		}
	} else {
		if (wrap && (len = ep - p))
			WR(p, len);
		if ((len = p - sp) != 0)
			WR(sp, len);
	}
	return 0;
}

/*
 * displaylines -- read lines to an offset from the end and display.
 *
 * This is the function that reads to a line offset from the end of the input,
 * storing the data in an array of buffers which is then displayed.  If the
 * rflag is set, the data is displayed in lines in reverse order, and this
 * routine has the usual nastiness of trying to find the newlines.  Otherwise,
 * it is displayed from the line closest to the beginning of the input to
 * the end.
 *
 * Non-zero return means than a (non-fatal) error occurred.
 */
int
displaylines(FILE *fp, off_t off)
{
	struct {
		int blen;
		int len;
		char *l;
	} *lines;
	int ch;
	char *p;
	int blen, cnt, recno, wrap;
	char *sp, *n;

	p = NULL;
	if ((lines = malloc(off * sizeof(*lines))) == NULL)
		xerr(1, "malloc");

	memset(lines, 0, sizeof(*lines) * off);

	sp = NULL;
	blen = cnt = recno = wrap = 0;

	while ((ch = getc(fp)) != EOF) {
		if (++cnt > blen) {
			if ((n = realloc(sp, blen + 1024)) == NULL)
				xerr(1, "realloc");
			sp = n;
			blen += 1024;
			p = sp + cnt - 1;
		}
		*p++ = ch;
		if (ch == '\n') {
			if (lines[recno].blen < cnt) {
				if ((n = realloc(lines[recno].l,
				    cnt + 256)) == NULL)
					xerr(1, "realloc");
				lines[recno].l = n;
				lines[recno].blen = cnt + 256;
			}
			memmove(lines[recno].l, sp, lines[recno].len = cnt);
			cnt = 0;
			p = sp;
			if (++recno == off) {
				wrap = 1;
				recno = 0;
			}
		}
	}
	if (ferror(fp)) {
		free(lines);
		ierr();
		return 1;
	}
	if (cnt) {
		lines[recno].l = sp;
		lines[recno].len = cnt;
		if (++recno == off) {
			wrap = 1;
			recno = 0;
		}
	}

	if (rflag) {
		for (cnt = recno - 1; cnt >= 0; --cnt)
			WR(lines[cnt].l, lines[cnt].len);
		if (wrap)
			for (cnt = off - 1; cnt >= recno; --cnt)
				WR(lines[cnt].l, lines[cnt].len);
	} else {
		if (wrap)
			for (cnt = recno; cnt < off; ++cnt)
				WR(lines[cnt].l, lines[cnt].len);
		for (cnt = 0; cnt < recno; ++cnt)
			WR(lines[cnt].l, lines[cnt].len);
	}
	free(lines);
	return 0;
}
