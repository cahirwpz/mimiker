/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

static off_t ifull, ofull, ipart, opart;

static void
usage(void)
{
	eprintf("usage: %s [operand...]\n", argv0);
}

static size_t
parsesize(char *expr)
{
	char *s = expr;
	size_t n = 1;

	for (;;) {
		n *= strtoumax(s, &s, 10);
		switch (*s) {
		case 'k': n <<= 10; s++; break;
		case 'b': n <<= 9; s++; break;
		}
		if (*s != 'x' || !s[1])
			break;
		s++;
	}
	if (*s || n == 0)
		eprintf("invalid block size expression '%s'\n", expr);

	return n;
}

static void
bswap(unsigned char *buf, size_t len)
{
	int c;

	for (len &= ~1; len > 0; buf += 2, len -= 2) {
		c = buf[0];
		buf[0] = buf[1];
		buf[1] = c;
	}
}

static void
lcase(unsigned char *buf, size_t len)
{
	for (; len > 0; buf++, len--)
		buf[0] = tolower(buf[0]);
}

static void
ucase(unsigned char *buf, size_t len)
{
	for (; len > 0; buf++, len--)
		buf[0] = toupper(buf[0]);
}

static void
summary(void)
{
	fprintf(stderr, "%"PRIdMAX"+%"PRIdMAX" records in\n", (intmax_t)ifull, (intmax_t)ipart);
	fprintf(stderr, "%"PRIdMAX"+%"PRIdMAX" records out\n", (intmax_t)ofull, (intmax_t)opart);
}

int
main(int argc, char *argv[])
{
	enum {
		LCASE   = 1 << 0,
		UCASE   = 1 << 1,
		SWAB    = 1 << 2,
		NOERROR = 1 << 3,
		NOTRUNC = 1 << 4,
		SYNC    = 1 << 5,
	} conv = 0;
	char *arg, *val, *end;
	const char *iname = "-", *oname = "-";
	int ifd = 0, ofd = 1, eof = 0;
	size_t len, bs = 0, ibs = 512, obs = 512, ipos = 0, opos = 0;
	off_t skip = 0, seek = 0, count = -1;
	ssize_t ret;
	unsigned char *buf;

	argv0 = argc ? (argc--, *argv++) : "dd";
	for (; argc > 0; argc--, argv++) {
		arg = *argv;
		val = strchr(arg, '=');
		if (!val)
			usage();
		*val++ = '\0';
		if (strcmp(arg, "if") == 0) {
			iname = val;
		} else if (strcmp(arg, "of") == 0) {
			oname = val;
		} else if (strcmp(arg, "ibs") == 0) {
			ibs = parsesize(val);
		} else if (strcmp(arg, "obs") == 0) {
			obs = parsesize(val);
		} else if (strcmp(arg, "bs") == 0) {
			bs = parsesize(val);
		} else if (strcmp(arg, "skip") == 0) {
			skip = estrtonum(val, 0, LLONG_MAX);
		} else if (strcmp(arg, "seek") == 0) {
			seek = estrtonum(val, 0, LLONG_MAX);
		} else if (strcmp(arg, "count") == 0) {
			count = estrtonum(val, 0, LLONG_MAX);
		} else if (strcmp(arg, "conv") == 0) {
			do {
				end = strchr(val, ',');
				if (end)
					*end++ = '\0';
				if (strcmp(val, "lcase") == 0)
					conv |= LCASE;
				else if (strcmp(val, "ucase") == 0)
					conv |= UCASE;
				else if (strcmp(val, "swab") == 0)
					conv |= SWAB;
				else if (strcmp(val, "noerror") == 0)
					conv |= NOERROR;
				else if (strcmp(val, "notrunc") == 0)
					conv |= NOTRUNC;
				else if (strcmp(val, "sync") == 0)
					conv |= SYNC;
				else
					eprintf("unknown conv flag '%s'\n", val);
				val = end;
			} while (val);
		} else {
			weprintf("unknown operand '%s'\n", arg);
			usage();
		}
	}

	if (bs)
		ibs = obs = bs;
	if (strcmp(iname, "-") != 0) {
		ifd = open(iname, O_RDONLY);
		if (ifd < 0)
			eprintf("open %s:", iname);
	}
	if (strcmp(oname, "-") != 0) {
		ofd = open(oname, O_WRONLY | O_CREAT | (conv & NOTRUNC || seek ? 0 : O_TRUNC), 0666);
		if (ofd < 0)
			eprintf("open %s:", oname);
	}

	len = MAX(ibs, obs) + ibs;
	buf = emalloc(len);
	if (skip && lseek(ifd, skip * ibs, SEEK_SET) < 0) {
		while (skip--) {
			ret = read(ifd, buf, ibs);
			if (ret < 0)
				eprintf("read:");
			if (ret == 0) {
				eof = 1;
				break;
			}
		}
	}
	if (seek) {
		if (!(conv & NOTRUNC) && ftruncate(ofd, seek * ibs) != 0)
			eprintf("ftruncate:");
		if (lseek(ofd, seek * ibs, SEEK_SET) < 0)
			eprintf("lseek:");
		/* XXX: handle non-seekable files */
	}
	while (!eof && (count == -1 || ifull + ipart < count)) {
		while (ipos - opos < obs) {
			ret = read(ifd, buf + ipos, ibs);
			if (ret == 0) {
				eof = 1;
				break;
			}
			if (ret < 0) {
				weprintf("read:");
				if (!(conv & NOERROR))
					return 1;
				summary();
				if (!(conv & SYNC))
					continue;
				ret = 0;
			}
			if (ret < ibs) {
				ipart++;
				if (conv & SYNC) {
					memset(buf + ipos + ret, 0, ibs - ret);
					ret = ibs;
				}
			} else {
				ifull++;
			}
			if (conv & SWAB)
				bswap(buf + ipos, ret);
			if (conv & LCASE)
				lcase(buf + ipos, ret);
			if (conv & UCASE)
				ucase(buf + ipos, ret);
			ipos += ret;
			if (bs && !(conv & (SWAB | LCASE | UCASE)))
				break;
		}
		if (ipos == opos)
			break;
		do {
			ret = write(ofd, buf + opos, MIN(obs, ipos - opos));
			if (ret < 0)
				eprintf("write:");
			if (ret == 0)
				eprintf("write returned 0\n");
			if (ret < obs)
				opart++;
			else
				ofull++;
			opos += ret;
		} while ((eof && ipos < opos) || (!eof && ipos - opos >= obs));
		if (opos < ipos)
			memmove(buf, buf + opos, ipos - opos);
		ipos -= opos;
		opos = 0;
	}
	summary();

	return 0;
}
