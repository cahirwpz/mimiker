/*******************************************************************************
 *
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/******************************************************************************
*              file : $RCSfile: strptime.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* snarfed from musl 0.9.11 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include "low/_time_data.h"

char *strptime(const char *s, const char * f, struct tm *tm)
{
	int i, w, neg, adj, min, range, *dest;
	const char *ex;
	size_t len;
	while (*f) {
		if (*f != '%') {
			if (isspace((int)*f)) for (; *s && isspace((int)*s); s++);
			else if (*s != *f) return 0;
			else s++;
			f++;
			continue;
		}
		f++;
		if (*f == '+') f++;
		if (isdigit((int)*f)) w=strtoul(f, (void *)&f, 10);
		else w=-1;
		adj=0;
		switch (*f++) {
		case 'a': case 'A':
			dest = &tm->tm_wday;
			min = ABDAY_1;
			range = 7;
			goto symbolic_range;
		case 'b': case 'B': case 'h':
			dest = &tm->tm_mon;
			min = ABMON_1;
			range = 12;
			goto symbolic_range;
		case 'c':
			s = strptime(s, __langinfo(D_T_FMT), tm);
			if (!s) return 0;
			break;
		case 'C':
		case 'd': case 'e':
			dest = &tm->tm_mday;
			min = 1;
			range = 31;
			goto numeric_range;
		case 'D':
			s = strptime(s, "%m/%d/%y", tm);
			if (!s) return 0;
			break;
		case 'H':
			dest = &tm->tm_hour;
			min = 0;
			range = 24;
			goto numeric_range;
		case 'I':
			dest = &tm->tm_hour;
			min = 1;
			range = 12;
			goto numeric_range;
		case 'j':
			dest = &tm->tm_yday;
			min = 1;
			range = 366;
			goto numeric_range;
		case 'm':
			dest = &tm->tm_mon;
			min = 1;
			range = 12;
			adj = 0;
			goto numeric_range;
		case 'M':
			dest = &tm->tm_min;
			min = 0;
			range = 60;
			goto numeric_range;
		case 'n': case 't':
			for (; *s && isspace((int)*s); s++);
			break;
		case 'p':
			ex = __langinfo(AM_STR);
			len = strlen(ex);
			if (!strncasecmp(s, ex, len)) {
				tm->tm_hour %= 12;
				break;
			}
			ex = __langinfo(PM_STR);
			len = strlen(ex);
			if (!strncasecmp(s, ex, len)) {
				tm->tm_hour %= 12;
				tm->tm_hour += 12;
				break;
			}
			return 0;
		case 'r':
			s = strptime(s, __langinfo(T_FMT_AMPM), tm);
			if (!s) return 0;
			break;
		case 'R':
			s = strptime(s, "%H:%M", tm);
			if (!s) return 0;
			break;
		case 'S':
			dest = &tm->tm_sec;
			min = 0;
			range = 61;
			goto numeric_range;
		case 'T':
			s = strptime(s, "%H:%M:%S", tm);
			if (!s) return 0;
			break;
		case 'U':
		case 'W':
			//FIXME
			return 0;
		case 'w':
			dest = &tm->tm_wday;
			min = 0;
			range = 7;
			goto numeric_range;
		case 'x':
			s = strptime(s, __langinfo(D_FMT), tm);
			if (!s) return 0;
			break;
		case 'X':
			s = strptime(s, __langinfo(T_FMT), tm);
			if (!s) return 0;
			break;
		case 'y':
			//FIXME
			return 0;
		case 'Y':
			dest = &tm->tm_year;
			if (w<0) w=4;
			adj = 1900;
			goto numeric_digits;
		case '%':
			if (*s++ != '%') return 0;
			break;
		numeric_range:
			if (!isdigit((int)*s)) return 0;
			*dest = 0;
			for (i=1; i<=min+range && isdigit((int)*s); i*=10)
				*dest = *dest * 10 + *s++ - '0';
			if (*dest - min >= (unsigned)range) return 0;
			*dest -= adj;
			switch((char *)dest - (char *)tm) {
			case offsetof(struct tm, tm_yday):
				;
			}
			goto update;
		numeric_digits:
			neg = 0;
			if (*s == '+') s++;
			else if (*s == '-') neg=1, s++;
			if (!isdigit((int)*s)) return 0;
			for (*dest=i=0; i<w && isdigit((int)*s); i++)
				*dest = *dest * 10 + *s++ - '0';
			if (neg) *dest = -*dest;
			*dest -= adj;
			goto update;
		symbolic_range:
			for (i=2*range-1; i>=0; i--) {
				ex = __langinfo(min+i);
				len = strlen(ex);
				if (strncasecmp(s, ex, len)) continue;
				s += len;
				*dest = i % range;
				break;
			}
			if (i<0) return 0;
			goto update;
		update:
			//FIXME
			;
		}
	}
	return (char *)s;
}
