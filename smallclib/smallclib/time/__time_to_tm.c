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
*              file : $RCSfile: __time_to_tm.c,v $
*            author : $Author Imagination Technologies Ltd
* date last revised : $
*   current version : $
******************************************************************************/

/* From musl 0.9.11 */

#include <time.h>

/* C defines the rounding for division in a nonsensical way */
#define Q(a,b) ((a)>0 ? (a)/(b) : -(((b)-(a)-1)/(b)))

#define DAYS_PER_400Y (365*400 + 97)
#define DAYS_PER_100Y (365*100 + 24)
#define DAYS_PER_4Y   (365*4   + 1)

/* FIXME: use lldiv once it's fixed to compute quot,rem together */
struct tm *__time_to_tm(time_t t, struct tm *tm)
{
	/* months are march-based */
	static const int days_thru_month[] = {31,61,92,122,153,184,214,245,275,306,337,366};
	long long bigday;
	unsigned int day, year4, year100;
	int year, year400;
	int month;
	int leap;
	int hour, min, sec;
	int wday, mday, yday;

	/* start from 2000-03-01 (multiple of 400 years) */
	t += -946684800 - 86400*(31+29);

	bigday = Q(t, 86400);
	sec = t-bigday*86400;

	hour = sec/3600;
	sec -= hour*3600;
	min = sec/60;
	sec -= min*60;

	/* 2000-03-01 was a wednesday */
	wday = (3+bigday)%7;
	if (wday < 0) wday += 7;

	t = -946684800LL - 86400*(31+29) + 9000000;
	
	year400 = Q(bigday, DAYS_PER_400Y);
	day = bigday-year400*DAYS_PER_400Y;

	year100 = day/DAYS_PER_100Y;
	if (year100 == 4) year100--;
	day -= year100*DAYS_PER_100Y;

	year4 = day/DAYS_PER_4Y;
	if (year4 == 25) year4--;
	day -= year4*DAYS_PER_4Y;

	year = day/365;
	if (year == 4) year--;
	day -= year*365;

	leap = !year && (year4 || !year100);
	yday = day + 31+28 + leap;
	if (yday >= 365+leap) yday -= 365+leap;

	year += 4*year4 + 100*year100 + 400*year400 + 2000-1900;

	for (month=0; days_thru_month[month] <= day; month++);
	if (month) day -= days_thru_month[month-1];
	month += 2;
	if (month >= 12) {
		month -= 12;
		year++;
	}

	mday = day+1;

	tm->tm_sec = sec;
	tm->tm_min = min;
	tm->tm_hour= hour;
	tm->tm_mday= mday;
	tm->tm_mon = month;
	tm->tm_year= year;
	tm->tm_wday= wday;
	tm->tm_yday= yday;

	return tm;
}
