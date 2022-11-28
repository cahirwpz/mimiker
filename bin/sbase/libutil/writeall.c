/* See LICENSE file for copyright and license details. */
#include <unistd.h>

#include "../util.h"

ssize_t
writeall(int fd, const void *buf, size_t len)
{
	const char *p = buf;
	ssize_t n;

	while (len) {
		n = write(fd, p, len);
		if (n <= 0)
			return n;
		p += n;
		len -= n;
	}

	return p - (const char *)buf;
}
