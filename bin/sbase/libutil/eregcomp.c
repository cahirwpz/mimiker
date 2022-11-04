#include <sys/types.h>

#include <regex.h>
#include <stdio.h>

#include "../util.h"

int
enregcomp(int status, regex_t *preg, const char *regex, int cflags)
{
	char errbuf[BUFSIZ] = "";
	int r;

	if ((r = regcomp(preg, regex, cflags)) == 0)
		return r;

	regerror(r, preg, errbuf, sizeof(errbuf));
	enprintf(status, "invalid regex: %s\n", errbuf);

	return r;
}

int
eregcomp(regex_t *preg, const char *regex, int cflags)
{
	return enregcomp(1, preg, regex, cflags);
}
