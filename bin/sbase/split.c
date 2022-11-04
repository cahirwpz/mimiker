/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

static int base = 26, start = 'a';

static int
itostr(char *str, int x, int n)
{
	str[n] = '\0';
	while (n-- > 0) {
		str[n] = start + (x % base);
		x /= base;
	}

	return x ? -1 : 0;
}

static FILE *
nextfile(FILE *f, char *buf, int plen, int slen)
{
	static int filecount = 0;

	if (f)
		fshut(f, "<file>");
	if (itostr(buf + plen, filecount++, slen) < 0)
		return NULL;

	if (!(f = fopen(buf, "w")))
		eprintf("'%s':", buf);

	return f;
}

static void
usage(void)
{
	eprintf("usage: %s [-a num] [-b num[k|m|g] | -l num] [-d] "
	        "[file [prefix]]\n", argv0);
}

int
main(int argc, char *argv[])
{
	FILE *in = stdin, *out = NULL;
	off_t size = 1000, n;
	int ret = 0, ch, plen, slen = 2, always = 0;
	char name[NAME_MAX + 1], *prefix = "x", *file = NULL;

	ARGBEGIN {
	case 'a':
		slen = estrtonum(EARGF(usage()), 0, INT_MAX);
		break;
	case 'b':
		always = 1;
		if ((size = parseoffset(EARGF(usage()))) < 0)
			return 1;
		if (!size)
			eprintf("size needs to be positive\n");
		break;
	case 'd':
		base = 10;
		start = '0';
		break;
	case 'l':
		always = 0;
		size = estrtonum(EARGF(usage()), 1, MIN(LLONG_MAX, SSIZE_MAX));
		break;
	default:
		usage();
	} ARGEND

	if (*argv)
		file = *argv++;
	if (*argv)
		prefix = *argv++;
	if (*argv)
		usage();

	plen = strlen(prefix);
	if (plen + slen > NAME_MAX)
		eprintf("names cannot exceed %d bytes\n", NAME_MAX);
	estrlcpy(name, prefix, sizeof(name));

	if (file && strcmp(file, "-")) {
		if (!(in = fopen(file, "r")))
			eprintf("fopen %s:", file);
	}

	n = 0;
	while ((ch = getc(in)) != EOF) {
		if (!out || n >= size) {
			if (!(out = nextfile(out, name, plen, slen)))
				eprintf("fopen: %s:", name);
			n = 0;
		}
		n += (always || ch == '\n');
		putc(ch, out);
	}

	ret |= (in != stdin) && fshut(in, "<infile>");
	ret |= out && (out != stdout) && fshut(out, "<outfile>");
	ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");

	return ret;
}
