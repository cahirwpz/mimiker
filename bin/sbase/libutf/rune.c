/* MIT/X Consortium Copyright (c) 2012 Connor Lane Smith <cls@lubutu.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "../utf.h"

#define MIN(x,y)  ((x) < (y) ? (x) : (y))

#define UTFSEQ(x) ((((x) & 0x80) == 0x00) ? 1 /* 0xxxxxxx */ \
                 : (((x) & 0xC0) == 0x80) ? 0 /* 10xxxxxx */ \
                 : (((x) & 0xE0) == 0xC0) ? 2 /* 110xxxxx */ \
                 : (((x) & 0xF0) == 0xE0) ? 3 /* 1110xxxx */ \
                 : (((x) & 0xF8) == 0xF0) ? 4 /* 11110xxx */ \
                 : (((x) & 0xFC) == 0xF8) ? 5 /* 111110xx */ \
                 : (((x) & 0xFE) == 0xFC) ? 6 /* 1111110x */ \
                                          : 0 )

#define BADRUNE(x) ((x) < 0 || (x) > Runemax \
                || ((x) & 0xFFFE) == 0xFFFE \
                || ((x) >= 0xD800 && (x) <= 0xDFFF) \
                || ((x) >= 0xFDD0 && (x) <= 0xFDEF))

int
runetochar(char *s, const Rune *p)
{
	Rune r = *p;

	switch(runelen(r)) {
	case 1: /* 0aaaaaaa */
		s[0] = r;
		return 1;
	case 2: /* 00000aaa aabbbbbb */
		s[0] = 0xC0 | ((r & 0x0007C0) >>  6); /* 110aaaaa */
		s[1] = 0x80 |  (r & 0x00003F);        /* 10bbbbbb */
		return 2;
	case 3: /* aaaabbbb bbcccccc */
		s[0] = 0xE0 | ((r & 0x00F000) >> 12); /* 1110aaaa */
		s[1] = 0x80 | ((r & 0x000FC0) >>  6); /* 10bbbbbb */
		s[2] = 0x80 |  (r & 0x00003F);        /* 10cccccc */
		return 3;
	case 4: /* 000aaabb bbbbcccc ccdddddd */
		s[0] = 0xF0 | ((r & 0x1C0000) >> 18); /* 11110aaa */
		s[1] = 0x80 | ((r & 0x03F000) >> 12); /* 10bbbbbb */
		s[2] = 0x80 | ((r & 0x000FC0) >>  6); /* 10cccccc */
		s[3] = 0x80 |  (r & 0x00003F);        /* 10dddddd */
		return 4;
	default:
		return 0; /* error */
	}
}

int
chartorune(Rune *p, const char *s)
{
	return charntorune(p, s, UTFmax);
}

int
charntorune(Rune *p, const char *s, size_t len)
{
	unsigned int i, n;
	Rune r;

	if(len == 0) /* can't even look at s[0] */
		return 0;

	switch((n = UTFSEQ(s[0]))) {
	case 1: r = s[0];        break; /* 0xxxxxxx */
	case 2: r = s[0] & 0x1F; break; /* 110xxxxx */
	case 3: r = s[0] & 0x0F; break; /* 1110xxxx */
	case 4: r = s[0] & 0x07; break; /* 11110xxx */
	case 5: r = s[0] & 0x03; break; /* 111110xx */
	case 6: r = s[0] & 0x01; break; /* 1111110x */
	default: /* invalid sequence */
		*p = Runeerror;
		return 1;
	}
	/* add values from continuation bytes */
	for(i = 1; i < MIN(n, len); i++)
		if((s[i] & 0xC0) == 0x80) {
			/* add bits from continuation byte to rune value
			 * cannot overflow: 6 byte sequences contain 31 bits */
			r = (r << 6) | (s[i] & 0x3F); /* 10xxxxxx */
		}
		else { /* expected continuation */
			*p = Runeerror;
			return i;
		}

	if(i < n) /* must have reached len limit */
		return 0;

	/* reject invalid or overlong sequences */
	if(BADRUNE(r) || runelen(r) < (int)n)
		r = Runeerror;

	*p = r;
	return n;
}

int
runelen(Rune r)
{
	if(BADRUNE(r))
		return 0; /* error */
	else if(r <= 0x7F)
		return 1;
	else if(r <= 0x07FF)
		return 2;
	else if(r <= 0xFFFF)
		return 3;
	else
		return 4;
}

size_t
runenlen(const Rune *p, size_t len)
{
	size_t i, n = 0;

	for(i = 0; i < len; i++)
		n += runelen(p[i]);
	return n;
}

int
fullrune(const char *s, size_t len)
{
	Rune r;

	return charntorune(&r, s, len) > 0;
}
