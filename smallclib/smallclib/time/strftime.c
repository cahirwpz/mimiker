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
*              file : $RCSfile: strftime.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* snarfed from musl 0.9.11 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "low/_time_data.h"

// FIXME: integer overflows


size_t strftime(char *s, size_t n, const char *f, const struct tm *tm)
{
	nl_item item;
	int val;
	const char *fmt;
	size_t l;
	for (l=0; *f && l<n; f++) {
		if (*f == '%') {
do_fmt:
		switch (*++f) {
		case '%':
			goto literal;
		case 'E':
		case 'O':
			goto do_fmt;
		case 'a':
			item = ABDAY_1 + tm->tm_wday;
			goto nl_strcat;
		case 'A':
			item = DAY_1 + tm->tm_wday;
			goto nl_strcat;
		case 'h':
		case 'b':
			item = ABMON_1 + tm->tm_mon;
			goto nl_strcat;
		case 'B':
			item = MON_1 + tm->tm_mon;
			goto nl_strcat;
		case 'c':
			item = D_T_FMT;
			goto nl_strftime;
		case 'C':
			val = (1900+tm->tm_year) / 100;
			fmt = "%02d";
			goto number;
		case 'd':
			val = tm->tm_mday;
			fmt = "%02d";
			goto number;
		case 'D':
			fmt = "%m/%d/%y";
			goto recu_strftime;
		case 'e':
			val = tm->tm_mday;
			fmt = "%2d";
			goto number;
		case 'F':
			fmt = "%Y-%m-%d";
			goto recu_strftime;
		case 'g':
			// FIXME
			val = 0; //week_based_year(tm)%100;
			fmt = "%02d";
			goto number;
		case 'G':
			// FIXME
			val = 0; //week_based_year(tm);
			fmt = "%04d";
			goto number;
		case 'H':
			val = tm->tm_hour;
			fmt = "%02d";
			goto number;
		case 'I':
			val = tm->tm_hour;
			if (!val) val = 12;
			else if (val > 12) val -= 12;
			fmt = "%02d";
			goto number;
		case 'j':
			val = tm->tm_yday+1;
			fmt = "%03d";
			goto number;
		case 'm':
			val = tm->tm_mon+1;
			fmt = "%02d";
			goto number;
		case 'M':
			val = tm->tm_min;
			fmt = "%02d";
			goto number;
		case 'n':
			s[l++] = '\n';
			continue;
		case 'p':
			item = tm->tm_hour >= 12 ? PM_STR : AM_STR;
			goto nl_strcat;
		case 'r':
			item = T_FMT_AMPM;
			goto nl_strftime;
		case 'R':
			fmt = "%H:%M";
			goto recu_strftime;
		case 'S':
			val = tm->tm_sec;
			fmt = "%02d";
			goto number;
		case 't':
			s[l++] = '\t';
			continue;
		case 'T':
			fmt = "%H:%M:%S";
			goto recu_strftime;
		case 'u':
			val = tm->tm_wday ? tm->tm_wday : 7;
			fmt = "%d";
			goto number;
		case 'U':
			val = (tm->tm_yday + 7 - tm->tm_wday) / 7;
			fmt = "%d";
			goto number;
		case 'V':
			// Not supported
			return 0;
		case 'W':
			// FIXME: week number mess..
            val = (tm->tm_yday + 7 - (tm->tm_wday+6)%7) / 7;
			fmt = "%d";
			goto number;
		case 'w':
			val = tm->tm_wday;
			fmt = "%d";
			goto number;
		case 'x':
			item = D_FMT;
			goto nl_strftime;
		case 'X':
			item = T_FMT;
			goto nl_strftime;
		case 'y':
			val = tm->tm_year % 100;
			fmt = "%02d";
			goto number;
		case 'Y':
			val = tm->tm_year + 1900;
			fmt = "%04d";
			goto number;
		case 'z':
			if (tm->tm_isdst < 0) continue;
			val = -__timezone - (tm->tm_isdst ? __dst_offset : 0);
			l += snprintf(s+l, n-l, "%+.2d%.2d", val/3600, abs(val%3600)/60);
			continue;
		case 'Z':
			if (tm->tm_isdst < 0 || !__tzname[0] || !__tzname[0][0])
				continue;
			l += snprintf(s+l, n-l, "%s", __tzname[!!tm->tm_isdst]);
			continue;
		default:
			return 0;
		}
		}
literal:
		s[l++] = *f;
		continue;
number:
		l += snprintf(s+l, n-l, fmt, val);
		continue;
nl_strcat:
		l += snprintf(s+l, n-l, "%s", __langinfo(item));
		continue;
nl_strftime:
		fmt = __langinfo(item);
recu_strftime:
		l += strftime(s+l, n-l, fmt, tm);
	}
	if (l >= n) return 0;
	s[l] = 0;
	return l;
}
