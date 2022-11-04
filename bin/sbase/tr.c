/* See LICENSE file for copyright and license details. */
#include <stdlib.h>

#include "utf.h"
#include "util.h"

static int cflag = 0;
static int dflag = 0;
static int sflag = 0;

struct range {
	Rune   start;
	Rune   end;
	size_t quant;
};

static struct {
	char    *name;
	int    (*check)(Rune);
} classes[] = {
	{ "alnum",  isalnumrune  },
	{ "alpha",  isalpharune  },
	{ "blank",  isblankrune  },
	{ "cntrl",  iscntrlrune  },
	{ "digit",  isdigitrune  },
	{ "graph",  isgraphrune  },
	{ "lower",  islowerrune  },
	{ "print",  isprintrune  },
	{ "punct",  ispunctrune  },
	{ "space",  isspacerune  },
	{ "upper",  isupperrune  },
	{ "xdigit", isxdigitrune },
};

static struct range *set1        = NULL;
static size_t set1ranges         = 0;
static int    (*set1check)(Rune) = NULL;
static struct range *set2        = NULL;
static size_t set2ranges         = 0;
static int    (*set2check)(Rune) = NULL;

static size_t
rangelen(struct range r)
{
	return (r.end - r.start + 1) * r.quant;
}

static size_t
setlen(struct range *set, size_t setranges)
{
	size_t len = 0, i;

	for (i = 0; i < setranges; i++)
		len += rangelen(set[i]);

	return len;
}

static int
rstrmatch(Rune *r, char *s, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++)
		if (r[i] != s[i])
			return 0;
	return 1;
}

static size_t
makeset(char *str, struct range **set, int (**check)(Rune))
{
	Rune  *rstr;
	size_t len, i, j, m, n;
	size_t q, setranges = 0;
	int    factor, base;

	/* rstr defines at most len ranges */
	unescape(str);
	rstr = ereallocarray(NULL, utflen(str) + 1, sizeof(*rstr));
	len = utftorunestr(str, rstr);
	*set = ereallocarray(NULL, len, sizeof(**set));

	for (i = 0; i < len; i++) {
		if (rstr[i] == '[') {
			j = i;
nextbrack:
			if (j >= len)
				goto literal;
			for (m = j; m < len; m++)
				if (rstr[m] == ']') {
					j = m;
					break;
				}
			if (j == i)
				goto literal;

			/* CLASSES [=EQUIV=] (skip) */
			if (j - i > 3 && rstr[i + 1] == '=' && rstr[m - 1] == '=') {
				if (j - i != 4)
					goto literal;
				(*set)[setranges].start = rstr[i + 2];
				(*set)[setranges].end   = rstr[i + 2];
				(*set)[setranges].quant = 1;
				setranges++;
				i = j;
				continue;
			}

			/* CLASSES [:CLASS:] */
			if (j - i > 3 && rstr[i + 1] == ':' && rstr[m - 1] == ':') {
				for (n = 0; n < LEN(classes); n++) {
					if (rstrmatch(rstr + i + 2, classes[n].name, j - i - 3)) {
						*check = classes[n].check;
						return 0;
					}
				}
				eprintf("Invalid character class.\n");
			}

			/* REPEAT  [_*n] (only allowed in set2) */
			if (j - i > 2 && rstr[i + 2] == '*') {
				/* check if right side of '*' is a number */
				q = 0;
				factor = 1;
				base = (rstr[i + 3] == '0') ? 8 : 10;
				for (n = j - 1; n > i + 2; n--) {
					if (rstr[n] < '0' || rstr[n] > '9') {
						n = 0;
						break;
					}
					q += (rstr[n] - '0') * factor;
					factor *= base;
				}
				if (n == 0) {
					j = m + 1;
					goto nextbrack;
				}
				(*set)[setranges].start = rstr[i + 1];
				(*set)[setranges].end   = rstr[i + 1];
				(*set)[setranges].quant = q ? q : setlen(set1, MAX(set1ranges, 1));
				setranges++;
				i = j;
				continue;
			}

			j = m + 1;
			goto nextbrack;
		}
literal:
		/* RANGES [_-__-_], _-__-_ */
		/* LITERALS _______ */
		(*set)[setranges].start = rstr[i];

		if (i < len - 2 && rstr[i + 1] == '-' && rstr[i + 2] >= rstr[i])
			i += 2;
		(*set)[setranges].end = rstr[i];
		(*set)[setranges].quant = 1;
		setranges++;
	}

	free(rstr);
	return setranges;
}

static void
usage(void)
{
	eprintf("usage: %s [-cCds] set1 [set2]\n", argv0);
}

int
main(int argc, char *argv[])
{
	Rune r, lastrune = 0;
	size_t off1, off2, i, m;
	int ret = 0;

	ARGBEGIN {
	case 'c':
	case 'C':
		cflag = 1;
		break;
	case 'd':
		dflag = 1;
		break;
	case 's':
		sflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if (!argc || argc > 2 || (argc == 1 && dflag == sflag))
		usage();
	set1ranges = makeset(argv[0], &set1, &set1check);
	if (argc == 2)
		set2ranges = makeset(argv[1], &set2, &set2check);

	if (!dflag || (argc == 2 && sflag)) {
		/* sanity checks as we are translating */
		if (!sflag && !set2ranges && !set2check)
			eprintf("cannot map to an empty set.\n");
		if (set2check && set2check != islowerrune &&
		    set2check != isupperrune) {
			eprintf("can only map to 'lower' and 'upper' class.\n");
		}
	}
read:
	if (!efgetrune(&r, stdin, "<stdin>")) {
		ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");
		return ret;
	}
	if (argc == 1 && sflag)
		goto write;
	for (i = 0, off1 = 0; i < set1ranges; off1 += rangelen(set1[i]), i++) {
		if (set1[i].start <= r && r <= set1[i].end) {
			if (dflag) {
				if (cflag)
					goto write;
				else
					goto read;
			}
			if (cflag)
				goto write;

			/* map r to set2 */
			if (set2check) {
				if (set2check == islowerrune)
					r = tolowerrune(r);
				else
					r = toupperrune(r);
			} else {
				off1 += r - set1[i].start;
				if (off1 > setlen(set2, set2ranges) - 1) {
					r = set2[set2ranges - 1].end;
					goto write;
				}
				for (m = 0, off2 = 0; m < set2ranges; m++) {
					if (off2 + rangelen(set2[m]) > off1) {
						m++;
						break;
					}
					off2 += rangelen(set2[m]);
				}
				m--;
				r = set2[m].start + (off1 - off2) / set2[m].quant;
			}
			goto write;
		}
	}
	if (set1check && set1check(r)) {
		if (dflag) {
			if (cflag)
				goto write;
			else
				goto read;
		}
		if (set2check) {
			if (set2check == islowerrune)
				r = tolowerrune(r);
			else
				r = toupperrune(r);
		} else {
			r = set2[set2ranges - 1].end;
		}
		goto write;
	}
	if (!dflag && cflag) {
		if (set2check) {
			if (set2check == islowerrune)
				r = tolowerrune(r);
			else
				r = toupperrune(r);
		} else {
			r = set2[set2ranges - 1].end;
		}
		goto write;
	}
	if (dflag && cflag)
		goto read;
write:
	if (argc == 1 && sflag && r == lastrune) {
		if (set1check && set1check(r))
			goto read;
		for (i = 0; i < set1ranges; i++) {
			if (set1[i].start <= r && r <= set1[i].end)
				goto read;
		}
	}
	if (argc == 2 && sflag && r == lastrune) {
		if (set2check && set2check(r))
			goto read;
		for (i = 0; i < set2ranges; i++) {
			if (set2[i].start <= r && r <= set2[i].end)
				goto read;
		}
	}
	efputrune(&r, stdout, "<stdout>");
	lastrune = r;
	goto read;
}
