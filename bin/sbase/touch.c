/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "util.h"

static int aflag;
static int cflag;
static int mflag;
static struct timespec times[2] = {{.tv_nsec = UTIME_NOW}};

static void
touch(const char *file)
{
	int fd, ret;

	if (utimensat(AT_FDCWD, file, times, 0) == 0)
		return;
	if (errno != ENOENT)
		eprintf("utimensat %s:", file);
	if (cflag)
		return;
	if ((fd = open(file, O_WRONLY | O_CREAT | O_EXCL, 0666)) < 0)
		eprintf("open %s:", file);
	ret = futimens(fd, times);
	close(fd);
	if (ret < 0)
		eprintf("futimens %s:", file);
}

static time_t
parsetime(char *str)
{
	time_t now;
	struct tm *cur, t = { 0 };
	int zulu = 0;
	char *format;
	size_t len = strlen(str);

	if ((now = time(NULL)) == -1)
		eprintf("time:");
	if (!(cur = localtime(&now)))
		eprintf("localtime:");
	t.tm_isdst = -1;

	switch (len) {
	/* -t flag argument */
	case 8:
		t.tm_year = cur->tm_year;
		format = "%m%d%H%M";
		break;
	case 10:
		format = "%y%m%d%H%M";
		break;
	case 11:
		t.tm_year = cur->tm_year;
		format = "%m%d%H%M.%S";
		break;
	case 12:
		format = "%Y%m%d%H%M";
		break;
	case 13:
		format = "%y%m%d%H%M.%S";
		break;
	case 15:
		format = "%Y%m%d%H%M.%S";
		break;
	/* -d flag argument */
	case 19:
		format = "%Y-%m-%dT%H:%M:%S";
		break;
	case 20:
		/* only Zulu-timezone supported */
		if (str[19] != 'Z')
			eprintf("Invalid time zone\n");
		str[19] = 0;
		zulu = 1;
		format = "%Y-%m-%dT%H:%M:%S";
		break;
	default:
		eprintf("Invalid date format length\n", str);
	}

	if (!strptime(str, format, &t))
		eprintf("strptime %s: Invalid date format\n", str);
	if (zulu) {
		t.tm_hour += t.tm_gmtoff / 60;
		t.tm_gmtoff = 0;
		t.tm_zone = "Z";
	}

	return mktime(&t);
}

static void
usage(void)
{
	eprintf("usage: %s [-acm] [-d time | -r ref_file | -t time | -T time] "
	        "file ...\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stat st;
	char *ref = NULL;

	ARGBEGIN {
	case 'a':
		aflag = 1;
		break;
	case 'c':
		cflag = 1;
		break;
	case 'd':
	case 't':
		times[0].tv_sec = parsetime(EARGF(usage()));
		times[0].tv_nsec = 0;
		break;
	case 'm':
		mflag = 1;
		break;
	case 'r':
		ref = EARGF(usage());
		if (stat(ref, &st) < 0)
			eprintf("stat '%s':", ref);
		times[0] = st.st_atim;
		times[1] = st.st_mtim;
		break;
	case 'T':
		times[0].tv_sec = estrtonum(EARGF(usage()), 0, LLONG_MAX);
		times[0].tv_nsec = 0;
		break;
	default:
		usage();
	} ARGEND

	if (!argc)
		usage();
	if (!aflag && !mflag)
		aflag = mflag = 1;
	if (!ref)
		times[1] = times[0];
	if (!aflag)
		times[0].tv_nsec = UTIME_OMIT;
	if (!mflag)
		times[1].tv_nsec = UTIME_OMIT;

	for (; *argv; argc--, argv++)
		touch(*argv);

	return 0;
}
