/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utf.h"
#include "util.h"

static char mode = 'n';

static int
dropinit(int fd, const char *fname, size_t count)
{
	Rune r;
	char buf[BUFSIZ], *p;
	ssize_t n;
	int nr;

	if (count < 2)
		goto copy;
	count--;  /* numbering starts at 1 */
	while (count && (n = read(fd, buf, sizeof(buf))) > 0) {
		switch (mode) {
		case 'n':  /* lines */
			for (p = buf; count && n > 0; p++, n--) {
				if (*p == '\n')
					count--;
			}
			break;
		case 'c':  /* bytes */
			if (count > n) {
				count -= n;
			} else {
				p = buf + count;
				n -= count;
				count = 0;
			}
			break;
		case 'm':  /* runes */
			for (p = buf; count && n > 0; p += nr, n -= nr, count--) {
				nr = charntorune(&r, p, n);
				if (!nr) {
					/* we don't have a full rune, move
					 * remaining data to beginning and read
					 * again */
					memmove(buf, p, n);
					break;
				}
			}
			break;
		}
	}
	if (count) {
		if (n < 0)
			weprintf("read %s:", fname);
		if (n <= 0)
			return n;
	}

	/* write the rest of the buffer */
	if (writeall(1, p, n) < 0)
		eprintf("write:");
copy:
	switch (concat(fd, fname, 1, "<stdout>")) {
	case -1:  /* read error */
		return -1;
	case -2:  /* write error */
		exit(1);
	default:
		return 0;
	}
}

static int
taketail(int fd, const char *fname, size_t count)
{
	static char *buf = NULL;
	static size_t size = 0;
	char *p;
	size_t len = 0, left;
	ssize_t n;

	if (!count)
		return 0;
	for (;;) {
		if (len + BUFSIZ > size) {
			/* make sure we have at least BUFSIZ to read */
			size += 2 * BUFSIZ;
			buf = erealloc(buf, size);
		}
		n = read(fd, buf + len, size - len);
		if (n < 0) {
			weprintf("read %s:", fname);
			return -1;
		}
		if (n == 0)
			break;
		len += n;
		switch (mode) {
		case 'n':  /* lines */
			/* ignore the last character; if it is a newline, it
			 * ends the last line */
			for (p = buf + len - 2, left = count; p >= buf; p--) {
				if (*p != '\n')
					continue;
				left--;
				if (!left) {
					p++;
					break;
				}
			}
			break;
		case 'c':  /* bytes */
			p = count < len ? buf + len - count : buf;
			break;
		case 'm':  /* runes */
			for (p = buf + len - 1, left = count; p >= buf; p--) {
				/* skip utf-8 continuation bytes */
				if ((*p & 0xc0) == 0x80)
					continue;
				left--;
				if (!left)
					break;
			}
			break;
		}
		if (p > buf) {
			len -= p - buf;
			memmove(buf, p, len);
		}
	}
	if (writeall(1, buf, len) < 0)
		eprintf("write:");
	return 0;
}

static void
usage(void)
{
	eprintf("usage: %s [-f] [-c num | -m num | -n num | -num] [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	struct stat st1, st2;
	int fd;
	size_t n = 10;
	int fflag = 0, ret = 0, newline = 0, many = 0;
	char *numstr;
	int (*tail)(int, const char *, size_t) = taketail;

	ARGBEGIN {
	case 'f':
		fflag = 1;
		break;
	case 'c':
	case 'm':
	case 'n':
		mode = ARGC();
		numstr = EARGF(usage());
		n = MIN(llabs(estrtonum(numstr, LLONG_MIN + 1,
		                        MIN(LLONG_MAX, SIZE_MAX))), SIZE_MAX);
		if (strchr(numstr, '+'))
			tail = dropinit;
		break;
	ARGNUM:
		n = ARGNUMF();
		break;
	default:
		usage();
	} ARGEND

	if (!argc) {
		if (tail(0, "<stdin>", n) < 0)
			ret = 1;
	} else {
		if ((many = argc > 1) && fflag)
			usage();
		for (newline = 0; *argv; argc--, argv++) {
			if (!strcmp(*argv, "-")) {
				*argv = "<stdin>";
				fd = 0;
			} else if ((fd = open(*argv, O_RDONLY)) < 0) {
				weprintf("open %s:", *argv);
				ret = 1;
				continue;
			}
			if (many)
				printf("%s==> %s <==\n", newline ? "\n" : "", *argv);
			if (fstat(fd, &st1) < 0)
				eprintf("fstat %s:", *argv);
			if (!(S_ISFIFO(st1.st_mode) || S_ISREG(st1.st_mode)))
				fflag = 0;
			newline = 1;
			if (tail(fd, *argv, n) < 0) {
				ret = 1;
				fflag = 0;
			}

			if (!fflag) {
				if (fd != 0)
					close(fd);
				continue;
			}
			for (;;) {
				if (concat(fd, *argv, 1, "<stdout>") < 0)
					exit(1);
				if (fstat(fd, &st2) < 0)
					eprintf("fstat %s:", *argv);
				if (st2.st_size < st1.st_size) {
					fprintf(stderr, "%s: file truncated\n", *argv);
					if (lseek(fd, SEEK_SET, 0) < 0)
						eprintf("lseek:");
				}
				st1 = st2;
				sleep(1);
			}
		}
	}

	return ret;
}
