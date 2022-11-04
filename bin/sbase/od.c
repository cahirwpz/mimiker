/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"
#include "util.h"

struct type {
	unsigned char     format;
	unsigned int      len;
	TAILQ_ENTRY(type) entry;
};

static TAILQ_HEAD(head, type) head = TAILQ_HEAD_INITIALIZER(head);
static unsigned char addr_format = 'o';
static off_t skip = 0;
static off_t max = -1;
static size_t linelen = 1;
static int big_endian;

static void
printaddress(off_t addr)
{
	char fmt[] = "%07j#";

	if (addr_format == 'n') {
		fputc(' ', stdout);
	} else {
		fmt[4] = addr_format;
		printf(fmt, (intmax_t)addr);
	}
}

static void
printchunk(const unsigned char *s, unsigned char format, size_t len)
{
	long long res, basefac;
	size_t i;
	char fmt[] = " %#*ll#";
	unsigned char c;

	const char *namedict[] = {
		"nul", "soh", "stx", "etx", "eot", "enq", "ack",
		"bel", "bs",  "ht",  "nl",  "vt",  "ff",  "cr",
		"so",  "si",  "dle", "dc1", "dc2", "dc3", "dc4",
		"nak", "syn", "etb", "can", "em",  "sub", "esc",
		"fs",  "gs",  "rs",  "us",  "sp",
	};
	const char *escdict[] = {
		['\0'] = "\\0", ['\a'] = "\\a",
		['\b'] = "\\b", ['\t'] = "\\t",
		['\n'] = "\\n", ['\v'] = "\\v",
		['\f'] = "\\f", ['\r'] = "\\r",
	};

	switch (format) {
	case 'a':
		c = *s & ~128; /* clear high bit as required by standard */
		if (c < LEN(namedict) || c == 127) {
			printf(" %3s", (c == 127) ? "del" : namedict[c]);
		} else {
			printf(" %3c", c);
		}
		break;
	case 'c':
		if (strchr("\a\b\t\n\v\f\r\0", *s)) {
			printf(" %3s", escdict[*s]);
		} else if (!isprint(*s)) {
			printf(" %3o", *s);
		} else {
			printf(" %3c", *s);
		}
		break;
	default:
		if (big_endian) {
			for (res = 0, basefac = 1, i = len; i; i--) {
				res += s[i - 1] * basefac;
				basefac <<= 8;
			}
		} else {
			for (res = 0, basefac = 1, i = 0; i < len; i++) {
				res += s[i] * basefac;
				basefac <<= 8;
			}
		}
		fmt[2] = big_endian ? '-' : ' ';
		fmt[6] = format;
		printf(fmt, (int)(3 * len + len - 1), res);
	}
}

static void
printline(const unsigned char *line, size_t len, off_t addr)
{
	struct type *t = NULL;
	size_t i;
	int first = 1;
	unsigned char *tmp;

	if (TAILQ_EMPTY(&head))
		goto once;
	TAILQ_FOREACH(t, &head, entry) {
once:
		if (first) {
			printaddress(addr);
			first = 0;
		} else {
			printf("%*c", (addr_format == 'n') ? 1 : 7, ' ');
		}
		for (i = 0; i < len; i += MIN(len - i, t ? t->len : 4)) {
			if (len - i < (t ? t->len : 4)) {
				tmp = ecalloc(t ? t->len : 4, 1);
				memcpy(tmp, line + i, len - i);
				printchunk(tmp, t ? t->format : 'o',
				           t ? t->len : 4);
				free(tmp);
			} else {
				printchunk(line + i, t ? t->format : 'o',
				           t ? t->len : 4);
			}
		}
		fputc('\n', stdout);
		if (TAILQ_EMPTY(&head) || (!len && !first))
			break;
	}
}

static int
od(int fd, char *fname, int last)
{
	static unsigned char *line;
	static size_t lineoff;
	static off_t addr;
	unsigned char buf[BUFSIZ];
	size_t i, size = sizeof(buf);
	ssize_t n;

	while (skip - addr > 0) {
		n = read(fd, buf, MIN(skip - addr, sizeof(buf)));
		if (n < 0)
			weprintf("read %s:", fname);
		if (n <= 0)
			return n;
		addr += n;
	}
	if (!line)
		line = emalloc(linelen);

	for (;;) {
		if (max >= 0)
			size = MIN(max - (addr - skip), size);
		if ((n = read(fd, buf, size)) <= 0)
			break;
		for (i = 0; i < n; i++, addr++) {
			line[lineoff++] = buf[i];
			if (lineoff == linelen) {
				printline(line, lineoff, addr - lineoff + 1);
				lineoff = 0;
			}
		}
	}
	if (n < 0) {
		weprintf("read %s:", fname);
		return n;
	}
	if (lineoff && last)
		printline(line, lineoff, addr - lineoff);
	if (last)
		printline((unsigned char *)"", 0, addr);
	return 0;
}

static int
lcm(unsigned int a, unsigned int b)
{
	unsigned int c, d, e;

	for (c = a, d = b; c ;) {
		e = c;
		c = d % c;
		d = e;
	}

	return a / d * b;
}

static void
addtype(char format, int len)
{
	struct type *t;

	t = emalloc(sizeof(*t));
	t->format = format;
	t->len = len;
	TAILQ_INSERT_TAIL(&head, t, entry);
}

static void
usage(void)
{
	eprintf("usage: %s [-bdosvx] [-A addressformat] [-E | -e] [-j skip] "
	        "[-t outputformat] [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int fd;
	struct type *t;
	int ret = 0, len;
	char *s;

	big_endian = (*(uint16_t *)"\0\xff" == 0xff);

	ARGBEGIN {
	case 'A':
		s = EARGF(usage());
		if (strlen(s) != 1 || !strchr("doxn", s[0]))
			usage();
		addr_format = s[0];
		break;
	case 'b':
		addtype('o', 1);
		break;
	case 'd':
		addtype('u', 2);
		break;
	case 'E':
	case 'e':
		big_endian = (ARGC() == 'E');
		break;
	case 'j':
		if ((skip = parseoffset(EARGF(usage()))) < 0)
			usage();
		break;
	case 'N':
		if ((max = parseoffset(EARGF(usage()))) < 0)
			usage();
		break;
	case 'o':
		addtype('o', 2);
		break;
	case 's':
		addtype('d', 2);
		break;
	case 't':
		s = EARGF(usage());
		for (; *s; s++) {
			switch (*s) {
			case 'a':
			case 'c':
				addtype(*s, 1);
				break;
			case 'd':
			case 'o':
			case 'u':
			case 'x':
				/* todo: allow multiple digits */
				if (*(s+1) > '0' && *(s+1) <= '9') {
					len = *(s+1) - '0';
				} else {
					switch (*(s+1)) {
					case 'C':
						len = sizeof(char);
						break;
					case 'S':
						len = sizeof(short);
						break;
					case 'I':
						len = sizeof(int);
						break;
					case 'L':
						len = sizeof(long);
						break;
					default:
						len = sizeof(int);
					}
				}
				addtype(*s++, len);
				break;
			default:
				usage();
			}
		}
		break;
	case 'v':
		/* always set - use uniq(1) to handle duplicate lines */
		break;
	case 'x':
		addtype('x', 2);
		break;
	default:
		usage();
	} ARGEND

	/* line length is lcm of type lengths and >= 16 by doubling */
	TAILQ_FOREACH(t, &head, entry)
		linelen = lcm(linelen, t->len);
	if (TAILQ_EMPTY(&head))
		linelen = 16;
	while (linelen < 16)
		linelen *= 2;

	if (!argc) {
		if (od(0, "<stdin>", 1) < 0)
			ret = 1;
	} else {
		for (; *argv; argc--, argv++) {
			if (!strcmp(*argv, "-")) {
				*argv = "<stdin>";
				fd = 0;
			} else if ((fd = open(*argv, O_RDONLY)) < 0) {
				weprintf("open %s:", *argv);
				ret = 1;
				continue;
			}
			if (od(fd, *argv, (!*(argv + 1))) < 0)
				ret = 1;
			if (fd != 0)
				close(fd);
		}
	}

	ret |= fshut(stdout, "<stdout>") | fshut(stderr, "<stderr>");

	return ret;
}
