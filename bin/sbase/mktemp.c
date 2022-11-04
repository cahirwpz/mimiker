/* See LICENSE file for copyright and license details. */
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static void
usage(void)
{
	eprintf("usage: %s [-dqtu] [-p directory] [template]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int dflag = 0, pflag = 0, qflag = 0, tflag = 0, uflag = 0, fd;
	char *template = "tmp.XXXXXXXXXX", *tmpdir = "", *pdir,
	     *p, path[PATH_MAX], tmp[PATH_MAX];
	size_t len;

	ARGBEGIN {
	case 'd':
		dflag = 1;
		break;
	case 'p':
		pflag = 1;
		pdir = EARGF(usage());
		break;
	case 'q':
		qflag = 1;
		break;
	case 't':
		tflag = 1;
		break;
	case 'u':
		uflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if (argc > 1)
		usage();
	else if (argc == 1)
		template = argv[0];

	if (!argc || pflag || tflag) {
		if ((p = getenv("TMPDIR")))
			tmpdir = p;
		else if (pflag)
			tmpdir = pdir;
		else
			tmpdir = "/tmp";
	}

	len = estrlcpy(path, tmpdir, sizeof(path));
	if (path[0] && path[len - 1] != '/')
		estrlcat(path, "/", sizeof(path));

	estrlcpy(tmp, template, sizeof(tmp));
	p = dirname(tmp);
	if (!(p[0] == '.' && p[1] == '\0')) {
		if (tflag && !pflag)
			eprintf("template must not contain directory separators in -t mode\n");
	}
	estrlcat(path, template, sizeof(path));

	if (dflag) {
		if (!mkdtemp(path)) {
			if (!qflag)
				eprintf("mkdtemp %s:", path);
			return 1;
		}
	} else {
		if ((fd = mkstemp(path)) < 0) {
			if (!qflag)
				eprintf("mkstemp %s:", path);
			return 1;
		}
		if (close(fd))
			eprintf("close %s:", path);
	}
	if (uflag)
		unlink(path);
	puts(path);

	efshut(stdout, "<stdout>");
	return 0;
}
