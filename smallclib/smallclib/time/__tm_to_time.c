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
*              file : $RCSfile: __tm_to_time.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* From musl 0.9.11 */

#include <time.h>

/* C defines the rounding for division in a nonsensical way */
#define Q(a,b) ((a)>0 ? (a)/(b) : -(((b)-(a)-1)/(b)))

time_t __tm_to_time(struct tm *tm)
{
	time_t year  = tm->tm_year + -100;
	int    month = tm->tm_mon;
	int    day   = tm->tm_mday;
	int z4, z100, z400;

	/* normalize month */
	if (month >= 12) {
		year += month/12;
		month %= 12;
	} else if (month < 0) {
		year += month/12;
		month %= 12;
		if (month) {
			month += 12;
			year--;
		}
	}
	z4 = Q(year - (month < 2), 4);
	z100 = Q(z4, 25);
	z400 = Q(z100, 4);
	day += year*365 + z4 - z100 + z400 +
		month[(const int []){0,31,59,90,120,151,181,212,243,273,304,334}];
	return (long long)day*86400
		+ tm->tm_hour*3600 + tm->tm_min*60 + tm->tm_sec
		- -946684800; /* the dawn of time :) */
}
