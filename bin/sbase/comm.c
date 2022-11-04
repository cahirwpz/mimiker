/* See LICENSE file for copyright and license details. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text.h"
#include "util.h"

static int show = 0x07;

static void
printline(int pos, struct line *line)
{
	int i;

	if (!(show & (0x1 << pos)))
		return;

	for (i = 0; i < pos; i++) {
		if (show & (0x1 << i))
			putchar('\t');
	}
	fwrite(line->data, 1, line->len, stdout);
}

static void
usage(void)
{
	eprintf("usage: %s [-123] file1 file2\n", argv0);
}

int
main(int argc, char *argv[])
{
	FILE *fp[2];
	static struct line line[2];
	size_t linecap[2] = { 0, 0 };
	ssize_t len;
	int ret = 0, i, diff = 0, seenline = 0;

	ARGBEGIN {
	case '1':
	case '2':
	case '3':
		show &= 0x07 ^ (1 << (ARGC() - '1'));
		break;
	default:
		usage();
	} ARGEND

	if (argc != 2)
		usage();

	for (i = 0; i < 2; i++) {
		if (!strcmp(argv[i], "-")) {
			argv[i] = "<stdin>";
			fp[i] = stdin;
		} else if (!(fp[i] = fopen(argv[i], "r"))) {
			eprintf("fopen %s:", argv[i]);
		}
	}

	for (;;) {
		for (i = 0; i < 2; i++) {
			if (diff && i == (diff < 0))
				continue;
			if ((len = getline(&(line[i].data), &linecap[i],
			                   fp[i])) > 0) {
				line[i].len = len;
				seenline = 1;
				continue;
			}
			if (ferror(fp[i]))
				eprintf("getline %s:", argv[i]);
			if ((diff || seenline) && line[!i].data[0])
				printline(!i, &line[!i]);
			while ((len = getline(&(line[!i].data), &linecap[!i],
			                      fp[!i])) > 0) {
				line[!i].len = len;
				printline(!i, &line[!i]);
			}
			if (ferror(fp[!i]))
				eprintf("getline %s:", argv[!i]);
			goto end;
		}
		diff = linecmp(&line[0], &line[1]);
		LIMIT(diff, -1, 1);
		seenline = 0;
		printline((2 - diff) % 3, &line[MAX(0, diff)]);
	}
end:
	ret |= fshut(fp[0], argv[0]);
	ret |= (fp[0] != fp[1]) && fshut(fp[1], argv[1]);
	ret |= fshut(stdout, "<stdout>");

	return ret;
}
