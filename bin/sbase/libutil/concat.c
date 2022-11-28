/* See LICENSE file for copyright and license details. */
#include <unistd.h>

#include "../util.h"

int
concat(int f1, const char *s1, int f2, const char *s2)
{
	char buf[BUFSIZ];
	ssize_t n;

	while ((n = read(f1, buf, sizeof(buf))) > 0) {
		if (writeall(f2, buf, n) < 0) {
			weprintf("write %s:", s2);
			return -2;
		}
	}
	if (n < 0) {
		weprintf("read %s:", s1);
		return -1;
	}
	return 0;
}
