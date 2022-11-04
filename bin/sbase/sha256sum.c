/* See LICENSE file for copyright and license details. */
#include <stdint.h>
#include <stdio.h>

#include "crypt.h"
#include "sha256.h"
#include "util.h"

static struct sha256 s;
struct crypt_ops sha256_ops = {
	sha256_init,
	sha256_update,
	sha256_sum,
	&s,
};

static void
usage(void)
{
	eprintf("usage: %s [-c] [file ...]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int ret = 0, (*cryptfunc)(int, char **, struct crypt_ops *, uint8_t *, size_t) = cryptmain;
	uint8_t md[SHA256_DIGEST_LENGTH];

	ARGBEGIN {
	case 'b':
	case 't':
		/* ignore */
		break;
	case 'c':
		cryptfunc = cryptcheck;
		break;
	default:
		usage();
	} ARGEND

	ret |= cryptfunc(argc, argv, &sha256_ops, md, sizeof(md));
	ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");

	return ret;
}
