/* See LICENSE file for copyright and license details. */
#include <sys/wait.h>

#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "util.h"

struct {
	const char *name;
	const int   sig;
} sigs[] = {
	{ "0", 0 },
#define SIG(n) { #n, SIG##n }
	SIG(ABRT), SIG(ALRM), SIG(BUS),  SIG(CHLD), SIG(CONT), SIG(FPE),  SIG(HUP),
	SIG(ILL),  SIG(INT),  SIG(KILL), SIG(PIPE), SIG(QUIT), SIG(SEGV), SIG(STOP),
	SIG(TERM), SIG(TSTP), SIG(TTIN), SIG(TTOU), SIG(USR1), SIG(USR2), SIG(URG),
#undef SIG
};

const char *
sig2name(const int sig)
{
	size_t i;

	for (i = 0; i < LEN(sigs); i++)
		if (sigs[i].sig == sig)
			return sigs[i].name;
	eprintf("%d: bad signal number\n", sig);

	return NULL; /* not reached */
}

int
name2sig(const char *name)
{
	size_t i;

	for (i = 0; i < LEN(sigs); i++)
		if (!strcasecmp(sigs[i].name, name))
			return sigs[i].sig;
	eprintf("%s: bad signal name\n", name);

	return -1; /* not reached */
}

static void
usage(void)
{
	eprintf("usage: %s [-s signame | -num | -signame] pid ...\n"
	        "       %s -l [num]\n", argv0, argv0);
}

int
main(int argc, char *argv[])
{
	pid_t pid;
	size_t i;
	int ret = 0, sig = SIGTERM;

	argv0 = *argv, argv0 ? (argc--, argv++) : (void *)0;

	if (!argc)
		usage();

	if ((*argv)[0] == '-') {
		switch ((*argv)[1]) {
		case 'l':
			if ((*argv)[2])
				goto longopt;
			argc--, argv++;
			if (!argc) {
				for (i = 0; i < LEN(sigs); i++)
					puts(sigs[i].name);
			} else if (argc == 1) {
				sig = estrtonum(*argv, 0, INT_MAX);
				if (sig > 128)
					sig = WTERMSIG(sig);
				puts(sig2name(sig));
			} else {
				usage();
			}
			return fshut(stdout, "<stdout>");
		case 's':
			if ((*argv)[2])
				goto longopt;
			argc--, argv++;
			if (!argc)
				usage();
			sig = name2sig(*argv);
			argc--, argv++;
			break;
		case '-':
			if ((*argv)[2])
				goto longopt;
			argc--, argv++;
			break;
		default:
		longopt:
			/* XSI-extensions -argnum and -argname*/
			if (isdigit((*argv)[1])) {
				sig = estrtonum((*argv) + 1, 0, INT_MAX);
				sig2name(sig);
			} else {
				sig = name2sig((*argv) + 1);
			}
			argc--, argv++;
		}
	}

	if (argc && !strcmp(*argv, "--"))
		argc--, argv++;

	if (!argc)
		usage();

	for (; *argv; argc--, argv++) {
		pid = estrtonum(*argv, INT_MIN, INT_MAX);
		if (kill(pid, sig) < 0) {
			weprintf("kill %d:", pid);
			ret = 1;
		}
	}

	return ret;
}
