/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <string.h>

#include "../util.h"

#define is_odigit(c)  ('0' <= c && c <= '7')

size_t
unescape(char *s)
{
	static const char escapes[256] = {
		['"'] = '"',
		['\''] = '\'',
		['\\'] = '\\',
		['a'] = '\a',
		['b'] = '\b',
		['E'] = 033,
		['e'] = 033,
		['f'] = '\f',
		['n'] = '\n',
		['r'] = '\r',
		['t'] = '\t',
		['v'] = '\v'
	};
	size_t m, q;
	char *r, *w;

	for (r = w = s; *r;) {
		if (*r != '\\') {
			*w++ = *r++;
			continue;
		}
		r++;
		if (!*r) {
			eprintf("null escape sequence\n");
		} else if (escapes[(unsigned char)*r]) {
			*w++ = escapes[(unsigned char)*r++];
		} else if (is_odigit(*r)) {
			for (q = 0, m = 4; m && is_odigit(*r); m--, r++)
				q = q * 8 + (*r - '0');
			*w++ = MIN(q, 255);
		} else if (*r == 'x' && isxdigit(r[1])) {
			r++;
			for (q = 0, m = 2; m && isxdigit(*r); m--, r++)
				if (isdigit(*r))
					q = q * 16 + (*r - '0');
				else
					q = q * 16 + (tolower(*r) - 'a' + 10);
			*w++ = q;
		} else {
			eprintf("invalid escape sequence '\\%c'\n", *r);
		}
	}
	*w = '\0';

	return w - s;
}
