/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-u] [-d time] [+format | mmddHHMM[[CC]yy]]\n", argv0);
}

static int
datefield(const char *s, size_t i)
{
	if (!isdigit(s[i]) || !isdigit(s[i+1]))
		eprintf("invalid date format: %s\n", s);

	return (s[i] - '0') * 10 + (s[i+1] - '0');
}

static void
setdate(const char *s, struct tm *now)
{
	struct tm date;
	struct timespec ts;

	switch (strlen(s)) {
	case 8:
		date.tm_year = now->tm_year;
		break;
	case 10:
		date.tm_year = datefield(s, 8);
		if (date.tm_year < 69)
			date.tm_year += 100;
		break;
	case 12:
		date.tm_year = ((datefield(s, 8) - 19) * 100) + datefield(s, 10);
		break;
	default:
		eprintf("invalid date format: %s\n", s);
		break;
	}

	date.tm_mon = datefield(s, 0) - 1;
	date.tm_mday = datefield(s, 2);
	date.tm_hour = datefield(s, 4);
	date.tm_min = datefield(s, 6);
	date.tm_sec = 0;
	date.tm_isdst = -1;

	ts.tv_sec = mktime(&date);
	if (ts.tv_sec == -1)
		eprintf("mktime:");
	ts.tv_nsec = 0;

	if (clock_settime(CLOCK_REALTIME, &ts) == -1)
		eprintf("clock_settime:");
}

int
main(int argc, char *argv[])
{
	struct tm *now;
	time_t t;
	char buf[BUFSIZ], *fmt = "%a %b %e %H:%M:%S %Z %Y";

	t = time(NULL);
	if (t == -1)
		eprintf("time:");

	ARGBEGIN {
	case 'd':
		t = estrtonum(EARGF(usage()), 0, LLONG_MAX);
		break;
	case 'u':
		if (setenv("TZ", "UTC0", 1) < 0)
			eprintf("setenv:");
		break;
	default:
		usage();
	} ARGEND

	if (!(now = localtime(&t)))
		eprintf("localtime:");
	if (argc) {
		if (argc != 1)
			usage();
		if (argv[0][0] != '+') {
			setdate(argv[0], now);
			return 0;
		}
		fmt = &argv[0][1];
	}

	strftime(buf, sizeof(buf), fmt, now);
	puts(buf);

	return fshut(stdout, "<stdout>");
}
