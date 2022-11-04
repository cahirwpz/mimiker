/* See LICENSE file for copyright and license details. */
#include <sys/wait.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

#define NARGS 10000

static int inputc(void);
static void fillargbuf(int);
static int eatspace(void);
static int parsequote(int);
static int parseescape(void);
static char *poparg(void);
static void waitchld(void);
static void spawn(void);

static size_t argbsz;
static size_t argbpos;
static size_t maxargs = 0;
static int    nerrors = 0;
static int    rflag = 0, nflag = 0, tflag = 0, xflag = 0;
static char  *argb;
static char  *cmd[NARGS];
static char  *eofstr;

static int
inputc(void)
{
	int ch;

	ch = getc(stdin);
	if (ch == EOF && ferror(stdin))
		eprintf("getc <stdin>:");

	return ch;
}

static void
fillargbuf(int ch)
{
	if (argbpos >= argbsz) {
		argbsz = argbpos == 0 ? 1 : argbsz * 2;
		argb = erealloc(argb, argbsz);
	}
	argb[argbpos] = ch;
}

static int
eatspace(void)
{
	int ch;

	while ((ch = inputc()) != EOF) {
		switch (ch) {
		case ' ': case '\t': case '\n':
			break;
		default:
			ungetc(ch, stdin);
			return ch;
		}
	}
	return -1;
}

static int
parsequote(int q)
{
	int ch;

	while ((ch = inputc()) != EOF) {
		if (ch == q)
			return 0;
		if (ch != '\n') {
			fillargbuf(ch);
			argbpos++;
		}
	}

	return -1;
}

static int
parseescape(void)
{
	int ch;

	if ((ch = inputc()) != EOF) {
		fillargbuf(ch);
		argbpos++;
		return ch;
	}

	return -1;
}

static char *
poparg(void)
{
	int ch;

	argbpos = 0;
	if (eatspace() < 0)
		return NULL;
	while ((ch = inputc()) != EOF) {
		switch (ch) {
		case ' ': case '\t': case '\n':
			goto out;
		case '\'':
			if (parsequote('\'') < 0)
				eprintf("unterminated single quote\n");
			break;
		case '\"':
			if (parsequote('\"') < 0)
				eprintf("unterminated double quote\n");
			break;
		case '\\':
			if (parseescape() < 0)
				eprintf("backslash at EOF\n");
			break;
		default:
			fillargbuf(ch);
			argbpos++;
			break;
		}
	}
out:
	fillargbuf('\0');

	return (eofstr && !strcmp(argb, eofstr)) ? NULL : argb;
}

static void
waitchld(void)
{
	int status;

	wait(&status);
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 255)
			exit(124);
		if (WEXITSTATUS(status) == 127 ||
		    WEXITSTATUS(status) == 126)
			exit(WEXITSTATUS(status));
		if (status)
			nerrors++;
	}
	if (WIFSIGNALED(status))
		exit(125);
}

static void
spawn(void)
{
	int savederrno;
	int first = 1;
	char **p;

	if (tflag) {
		for (p = cmd; *p; p++) {
			if (!first)
				fputc(' ', stderr);
			fputs(*p, stderr);
			first = 0;
		}
		fputc('\n', stderr);
	}

	switch (fork()) {
	case -1:
		eprintf("fork:");
	case 0:
		execvp(*cmd, cmd);
		savederrno = errno;
		weprintf("execvp %s:", *cmd);
		_exit(126 + (savederrno == ENOENT));
	}
	waitchld();
}

static void
usage(void)
{
	eprintf("usage: %s [-rtx] [-E eofstr] [-n num] [-s num] "
	        "[cmd [arg ...]]\n", argv0);
}

int
main(int argc, char *argv[])
{
	int ret = 0, leftover = 0, i;
	size_t argsz, argmaxsz;
	size_t arglen, a;
	char *arg = "";

	if ((argmaxsz = sysconf(_SC_ARG_MAX)) == (size_t)-1)
		argmaxsz = _POSIX_ARG_MAX;
	/* Leave some room for environment variables */
	argmaxsz -= 4096;

	ARGBEGIN {
	case 'n':
		nflag = 1;
		maxargs = estrtonum(EARGF(usage()), 1, MIN(SIZE_MAX, LLONG_MAX));
		break;
	case 'r':
		rflag = 1;
		break;
	case 's':
		argmaxsz = estrtonum(EARGF(usage()), 1, MIN(SIZE_MAX, LLONG_MAX));
		break;
	case 't':
		tflag = 1;
		break;
	case 'x':
		xflag = 1;
		break;
	case 'E':
		eofstr = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	do {
		argsz = 0; i = 0; a = 0;
		if (argc) {
			for (; i < argc; i++) {
				cmd[i] = estrdup(argv[i]);
				argsz += strlen(cmd[i]) + 1;
			}
		} else {
			cmd[i] = estrdup("/bin/echo");
			argsz += strlen("/bin/echo") + 1;
			i++;
		}
		while (leftover || (arg = poparg())) {
			arglen = strlen(arg);
			if (argsz + arglen >= argmaxsz || i >= NARGS - 1) {
				if (arglen >= argmaxsz) {
					weprintf("insufficient argument space\n");
					if (xflag)
						exit(1);
				}
				leftover = 1;
				break;
			}
			cmd[i] = estrdup(arg);
			argsz += arglen + 1;
			i++;
			a++;
			leftover = 0;
			if (nflag && a >= maxargs)
				break;
		}
		cmd[i] = NULL;
		if (a >= maxargs && nflag)
			spawn();
		else if (!a || (i == 1 && rflag))
			;
		else
			spawn();
		for (; i >= 0; i--)
			free(cmd[i]);
	} while (arg);

	free(argb);

	if (nerrors || (fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>")))
		ret = 123;

	return ret;
}
