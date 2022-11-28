/* See LICENSE file for copyright and license details. */

struct line {
	char *data;
	size_t len;
};

struct linebuf {
	struct line *lines;
	size_t nlines;
	size_t capacity;
};
#define EMPTY_LINEBUF {NULL, 0, 0,}
void getlines(FILE *, struct linebuf *);

int linecmp(struct line *, struct line *);
