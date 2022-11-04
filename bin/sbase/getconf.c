/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

struct var {
	const char *k;
	long v;
};

#include "getconf.h"

void
usage(void)
{
	eprintf("usage: %s [-v spec] var [path]\n", argv0);
}

int
main(int argc, char *argv[])
{
	size_t len;
	long res;
	int i;
	char *str;

	ARGBEGIN {
	case 'v':
		/* ignore */
		EARGF(usage());
		break;
	default:
		usage();
		break;
	} ARGEND

	if (argc == 1) {
		/* sysconf */
		for (i = 0; i < LEN(sysconf_l); i++) {
			if (strcmp(argv[0], sysconf_l[i].k))
				continue;
			errno = 0;
			if ((res = sysconf(sysconf_l[i].v)) < 0) {
				if (errno)
					eprintf("sysconf %ld:", sysconf_l[i].v);
				puts("undefined");
			} else {
				printf("%ld\n", res);
			}
			return fshut(stdout, "<stdout>");
		}
		/* confstr */
		for (i = 0; i < LEN(confstr_l); i++) {
			if (strcmp(argv[0], confstr_l[i].k))
				continue;
			errno = 0;
			if (!(len = confstr(confstr_l[i].v, NULL, 0))) {
				if (errno)
					eprintf("confstr %ld:", confstr_l[i].v);
				puts("undefined");
			} else {
				str = emalloc(len);
				errno = 0;
				if (!confstr(confstr_l[i].v, str, len)) {
					if (errno)
						eprintf("confstr %ld:", confstr_l[i].v);
					puts("undefined");
				} else {
					puts(str);
				}
				free(str);
			}
			return fshut(stdout, "<stdout>");
		}
		/* limits */
		for (i = 0; i < LEN(limits_l); i++) {
			if (strcmp(argv[0], limits_l[i].k))
				continue;
			printf("%ld\n", limits_l[i].v);
			return fshut(stdout, "<stdout>");
		}
	} else if (argc == 2) {
		/* pathconf */
		for (i = 0; i < LEN(pathconf_l); i++) {
			if (strcmp(argv[0], pathconf_l[i].k))
				continue;
			errno = 0;
			if ((res = pathconf(argv[1], pathconf_l[i].v)) < 0) {
				if (errno)
					eprintf("pathconf %ld:", pathconf_l[i].v);
				puts("undefined");
			} else {
				printf("%ld\n", res);
			}
			return fshut(stdout, "<stdout>");
		}
	} else {
		usage();
	}

	eprintf("invalid variable: %s\n", argv[0]);

	return 0;
}
