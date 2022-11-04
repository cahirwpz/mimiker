/* See LICENSE file for copyright and license details. */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "util.h"

enum { JAN, FEB, MAR, APR, MAY, JUN, JUL, AUG, SEP, OCT, NOV, DEC };
enum caltype { JULIAN, GREGORIAN };
enum { TRANS_YEAR = 1752, TRANS_MONTH = SEP, TRANS_DAY = 2 };

static struct tm *ltime;

static int
isleap(size_t year, enum caltype cal)
{
	if (cal == GREGORIAN) {
		if (year % 400 == 0)
			return 1;
		if (year % 100 == 0)
			return 0;
		return (year % 4 == 0);
	}
	else { /* cal == Julian */
		return (year % 4 == 0);
	}
}

static int
monthlength(size_t year, int month, enum caltype cal)
{
	int mdays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	return (month == FEB && isleap(year, cal)) ? 29 : mdays[month];
}

/* From http://www.tondering.dk/claus/cal/chrweek.php#calcdow */
static int
dayofweek(size_t year, int month, int dom, enum caltype cal)
{
	size_t y;
	int m, a;

	a = (13 - month) / 12;
	y = year - a;
	m = month + 12 * a - 1;

	if (cal == GREGORIAN)
		return (dom + y + y / 4 - y / 100 + y / 400 + (31 * m) / 12) % 7;
	else  /* cal == Julian */
		return (5 + dom + y + y / 4 + (31 * m) / 12) % 7;
}

static void
printgrid(size_t year, int month, int fday, int line)
{
	enum caltype cal;
	int offset, dom, d = 0, trans; /* are we in the transition from Julian to Gregorian? */
	int today = 0;

	cal = (year < TRANS_YEAR || (year == TRANS_YEAR && month <= TRANS_MONTH)) ? JULIAN : GREGORIAN;
	trans = (year == TRANS_YEAR && month == TRANS_MONTH);
	offset = dayofweek(year, month, 1, cal) - fday;

	if (offset < 0)
		offset += 7;
	if (line == 1) {
		for (; d < offset; ++d)
			printf("   ");
		dom = 1;
	} else {
		dom = 8 - offset + (line - 2) * 7;
		if (trans && !(line == 2 && fday == 3))
			dom += 11;
	}
	if (ltime && year == ltime->tm_year + 1900 && month == ltime->tm_mon)
		today = ltime->tm_mday;
	for (; d < 7 && dom <= monthlength(year, month, cal); ++d, ++dom) {
		if (dom == today)
			printf("\x1b[7m%2d\x1b[0m ", dom); /* highlight today's date */
		else
			printf("%2d ", dom);
		if (trans && dom == TRANS_DAY)
			dom += 11;
	}
	for (; d < 7; ++d)
		printf("   ");
}

static void
drawcal(size_t year, int month, size_t ncols, size_t nmons, int fday)
{
	char *smon[] = { "January", "February", "March", "April",
	                 "May", "June", "July", "August",
	                 "September", "October", "November", "December" };
	char *days[] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa", };
	size_t m, n, col, cur_year, cur_month, dow;
	int line, pad;
	char month_year[sizeof("Su Mo Tu We Th Fr Sa")];

	for (m = 0; m < nmons; ) {
		n = m;
		for (col = 0; m < nmons && col < ncols; ++col, ++m) {
			cur_year = year + m / 12;
			cur_month = month + m % 12;
			if (cur_month > 11) {
				cur_month -= 12;
				cur_year += 1;
			}
			snprintf(month_year, sizeof(month_year), "%s %zu", smon[cur_month], cur_year);
			pad = sizeof(month_year) - 1 - strlen(month_year);
			printf("%*s%s%*s   ", pad / 2 + pad % 2, "", month_year, pad / 2, "");
		}
		putchar('\n');
		for (col = 0, m = n; m < nmons && col < ncols; ++col, ++m) {
			for (dow = fday; dow < (fday + 7); ++dow)
				printf("%s ", days[dow % 7]);
			printf("  ");
		}
		putchar('\n');
		for (line = 1; line <= 6; ++line) {
			for (col = 0, m = n; m < nmons && col < ncols; ++col, ++m) {
				cur_year = year + m / 12;
				cur_month = month + m % 12;
				if (cur_month > 11) {
					cur_month -= 12;
					cur_year += 1;
				}
				printgrid(cur_year, cur_month, fday, line);
				printf("  ");
			}
			putchar('\n');
		}
	}
}

static void
usage(void)
{
	eprintf("usage: %s [-1 | -3 | -y | -n num] "
	        "[-s | -m | -f num] [-c num] [[month] year]\n", argv0);
}

int
main(int argc, char *argv[])
{
	time_t now;
	size_t year, ncols, nmons;
	int fday, month;

	now   = time(NULL);
	ltime = localtime(&now);
	year  = ltime->tm_year + 1900;
	month = ltime->tm_mon + 1;
	fday  = 0;

	if (!isatty(STDOUT_FILENO))
		ltime = NULL; /* don't highlight today's date */

	ncols = 3;
	nmons = 0;

	ARGBEGIN {
	case '1':
		nmons = 1;
		break;
	case '3':
		nmons = 3;
		if (--month == 0) {
			month = 12;
			year--;
		}
		break;
	case 'c':
		ncols = estrtonum(EARGF(usage()), 0, MIN(SIZE_MAX, LLONG_MAX));
		break;
	case 'f':
		fday = estrtonum(EARGF(usage()), 0, 6);
		break;
	case 'm': /* Monday */
		fday = 1;
		break;
	case 'n':
		nmons = estrtonum(EARGF(usage()), 1, MIN(SIZE_MAX, LLONG_MAX));
		break;
	case 's': /* Sunday */
		fday = 0;
		break;
	case 'y':
		month = 1;
		nmons = 12;
		break;
	default:
		usage();
	} ARGEND

	if (nmons == 0) {
		if (argc == 1) {
			month = 1;
			nmons = 12;
		} else {
			nmons = 1;
		}
	}

	switch (argc) {
	case 2:
		month = estrtonum(argv[0], 1, 12);
		argv++;
	case 1: /* fallthrough */
		year = estrtonum(argv[0], 0, INT_MAX);
		break;
	case 0:
		break;
	default:
		usage();
	}

	drawcal(year, month - 1, ncols, nmons, fday);

	return fshut(stdout, "<stdout>");
}
