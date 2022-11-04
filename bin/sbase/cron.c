/* See LICENSE file for copyright and license details. */
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "queue.h"
#include "util.h"

struct field {
	enum {
		ERROR,
		WILDCARD,
		NUMBER,
		RANGE,
		REPEAT,
		LIST
	} type;
	long *val;
	int len;
};

struct ctabentry {
	struct field min;
	struct field hour;
	struct field mday;
	struct field mon;
	struct field wday;
	char *cmd;
	TAILQ_ENTRY(ctabentry) entry;
};

struct jobentry {
	char *cmd;
	pid_t pid;
	TAILQ_ENTRY(jobentry) entry;
};

static sig_atomic_t chldreap;
static sig_atomic_t reload;
static sig_atomic_t quit;
static TAILQ_HEAD(, ctabentry) ctabhead = TAILQ_HEAD_INITIALIZER(ctabhead);
static TAILQ_HEAD(, jobentry) jobhead = TAILQ_HEAD_INITIALIZER(jobhead);
static char *config = "/etc/crontab";
static char *pidfile = "/var/run/crond.pid";
static int nflag;

static void
loginfo(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (nflag == 0)
		vsyslog(LOG_INFO, fmt, ap);
	else
		vfprintf(stdout, fmt, ap);
	fflush(stdout);
	va_end(ap);
}

static void
logwarn(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (nflag == 0)
		vsyslog(LOG_WARNING, fmt, ap);
	else
		vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static void
logerr(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (nflag == 0)
		vsyslog(LOG_ERR, fmt, ap);
	else
		vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static void
runjob(char *cmd)
{
	struct jobentry *je;
	time_t t;
	pid_t pid;

	t = time(NULL);

	/* If command is already running, skip it */
	TAILQ_FOREACH(je, &jobhead, entry) {
		if (strcmp(je->cmd, cmd) == 0) {
			loginfo("already running %s pid: %d at %s",
				je->cmd, je->pid, ctime(&t));
			return;
		}
	}

	switch ((pid = fork())) {
	case -1:
		logerr("error: failed to fork job: %s time: %s",
		       cmd, ctime(&t));
		return;
	case 0:
		setsid();
		loginfo("run: %s pid: %d at %s",
			cmd, getpid(), ctime(&t));
		execl("/bin/sh", "/bin/sh", "-c", cmd, (char *)NULL);
		logerr("error: failed to execute job: %s time: %s",
		       cmd, ctime(&t));
		_exit(1);
	default:
		je = emalloc(sizeof(*je));
		je->cmd = estrdup(cmd);
		je->pid = pid;
		TAILQ_INSERT_TAIL(&jobhead, je, entry);
	}
}

static void
waitjob(void)
{
	struct jobentry *je, *tmp;
	int status;
	time_t t;
	pid_t pid;

	t = time(NULL);

	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
		je = NULL;
		TAILQ_FOREACH(tmp, &jobhead, entry) {
			if (tmp->pid == pid) {
				je = tmp;
				break;
			}
		}
		if (je) {
			TAILQ_REMOVE(&jobhead, je, entry);
			free(je->cmd);
			free(je);
		}
		if (WIFEXITED(status) == 1)
			loginfo("complete: pid: %d returned: %d time: %s",
				pid, WEXITSTATUS(status), ctime(&t));
		else if (WIFSIGNALED(status) == 1)
			loginfo("complete: pid: %d terminated by signal: %s time: %s",
				pid, strsignal(WTERMSIG(status)), ctime(&t));
		else if (WIFSTOPPED(status) == 1)
			loginfo("complete: pid: %d stopped by signal: %s time: %s",
				pid, strsignal(WSTOPSIG(status)), ctime(&t));
	}
}

static int
isleap(int year)
{
	if (year % 400 == 0)
		return 1;
	if (year % 100 == 0)
		return 0;
	return (year % 4 == 0);
}

static int
daysinmon(int mon, int year)
{
	int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (year < 1900)
		year += 1900;
	if (isleap(year))
		days[1] = 29;
	return days[mon];
}

static int
matchentry(struct ctabentry *cte, struct tm *tm)
{
	struct {
		struct field *f;
		int tm;
		int len;
	} matchtbl[] = {
		{ .f = &cte->min,  .tm = tm->tm_min,  .len = 60 },
		{ .f = &cte->hour, .tm = tm->tm_hour, .len = 24 },
		{ .f = &cte->mday, .tm = tm->tm_mday, .len = daysinmon(tm->tm_mon, tm->tm_year) },
		{ .f = &cte->mon,  .tm = tm->tm_mon,  .len = 12 },
		{ .f = &cte->wday, .tm = tm->tm_wday, .len = 7  },
	};
	size_t i;
	int j;

	for (i = 0; i < LEN(matchtbl); i++) {
		switch (matchtbl[i].f->type) {
		case WILDCARD:
			continue;
		case NUMBER:
			if (matchtbl[i].f->val[0] == matchtbl[i].tm)
				continue;
			break;
		case RANGE:
			if (matchtbl[i].f->val[0] <= matchtbl[i].tm)
				if (matchtbl[i].f->val[1] >= matchtbl[i].tm)
					continue;
			break;
		case REPEAT:
			if (matchtbl[i].tm > 0) {
				if (matchtbl[i].tm % matchtbl[i].f->val[0] == 0)
					continue;
			} else {
				if (matchtbl[i].len % matchtbl[i].f->val[0] == 0)
					continue;
			}
			break;
		case LIST:
			for (j = 0; j < matchtbl[i].f->len; j++)
				if (matchtbl[i].f->val[j] == matchtbl[i].tm)
					break;
			if (j < matchtbl[i].f->len)
				continue;
			break;
		default:
			break;
		}
		break;
	}
	if (i != LEN(matchtbl))
		return 0;
	return 1;
}

static int
parsefield(const char *field, long low, long high, struct field *f)
{
	int i;
	char *e1, *e2;
	const char *p;

	p = field;
	while (isdigit(*p))
		p++;

	f->type = ERROR;

	switch (*p) {
	case '*':
		if (strcmp(field, "*") == 0) {
			f->val = NULL;
			f->len = 0;
			f->type = WILDCARD;
		} else if (strncmp(field, "*/", 2) == 0) {
			f->val = emalloc(sizeof(*f->val));
			f->len = 1;

			errno = 0;
			f->val[0] = strtol(field + 2, &e1, 10);
			if (e1[0] != '\0' || errno != 0 || f->val[0] == 0)
				break;

			f->type = REPEAT;
		}
		break;
	case '\0':
		f->val = emalloc(sizeof(*f->val));
		f->len = 1;

		errno = 0;
		f->val[0] = strtol(field, &e1, 10);
		if (e1[0] != '\0' || errno != 0)
			break;

		f->type = NUMBER;
		break;
	case '-':
		f->val = emalloc(2 * sizeof(*f->val));
		f->len = 2;

		errno = 0;
		f->val[0] = strtol(field, &e1, 10);
		if (e1[0] != '-' || errno != 0)
			break;

		errno = 0;
		f->val[1] = strtol(e1 + 1, &e2, 10);
		if (e2[0] != '\0' || errno != 0)
			break;

		f->type = RANGE;
		break;
	case ',':
		for (i = 1; isdigit(*p) || *p == ','; p++)
			if (*p == ',')
				i++;
		f->val = emalloc(i * sizeof(*f->val));
		f->len = i;

		errno = 0;
		f->val[0] = strtol(field, &e1, 10);
		if (f->val[0] < low || f->val[0] > high)
			break;

		for (i = 1; *e1 == ',' && errno == 0; i++) {
			errno = 0;
			f->val[i] = strtol(e1 + 1, &e2, 10);
			e1 = e2;
		}
		if (e1[0] != '\0' || errno != 0)
			break;

		f->type = LIST;
		break;
	default:
		return -1;
	}

	for (i = 0; i < f->len; i++)
		if (f->val[i] < low || f->val[i] > high)
			f->type = ERROR;

	if (f->type == ERROR) {
		free(f->val);
		return -1;
	}

	return 0;
}

static void
freecte(struct ctabentry *cte, int nfields)
{
	switch (nfields) {
	case 6:
		free(cte->cmd);
	case 5:
		free(cte->wday.val);
	case 4:
		free(cte->mon.val);
	case 3:
		free(cte->mday.val);
	case 2:
		free(cte->hour.val);
	case 1:
		free(cte->min.val);
	}
	free(cte);
}

static void
unloadentries(void)
{
	struct ctabentry *cte, *tmp;

	for (cte = TAILQ_FIRST(&ctabhead); cte; cte = tmp) {
		tmp = TAILQ_NEXT(cte, entry);
		TAILQ_REMOVE(&ctabhead, cte, entry);
		freecte(cte, 6);
	}
}

static int
loadentries(void)
{
	struct ctabentry *cte;
	FILE *fp;
	char *line = NULL, *p, *col;
	int r = 0, y;
	size_t size = 0;
	ssize_t len;
	struct fieldlimits {
		char *name;
		long min;
		long max;
		struct field *f;
	} flim[] = {
		{ "min",  0, 59, NULL },
		{ "hour", 0, 23, NULL },
		{ "mday", 1, 31, NULL },
		{ "mon",  1, 12, NULL },
		{ "wday", 0, 6,  NULL }
	};
	size_t x;

	if ((fp = fopen(config, "r")) == NULL) {
		logerr("error: can't open %s: %s\n", config, strerror(errno));
		return -1;
	}

	for (y = 0; (len = getline(&line, &size, fp)) != -1; y++) {
		p = line;
		if (line[0] == '#' || line[0] == '\n' || line[0] == '\0')
			continue;

		cte = emalloc(sizeof(*cte));
		flim[0].f = &cte->min;
		flim[1].f = &cte->hour;
		flim[2].f = &cte->mday;
		flim[3].f = &cte->mon;
		flim[4].f = &cte->wday;

		for (x = 0; x < LEN(flim); x++) {
			do
				col = strsep(&p, "\t\n ");
			while (col && col[0] == '\0');

			if (!col || parsefield(col, flim[x].min, flim[x].max, flim[x].f) < 0) {
				logerr("error: failed to parse `%s' field on line %d\n",
						flim[x].name, y + 1);
				freecte(cte, x);
				r = -1;
				break;
			}
		}

		if (r == -1)
			break;

		col = strsep(&p, "\n");
		if (col)
			while (col[0] == '\t' || col[0] == ' ')
				col++;
		if (!col || col[0] == '\0') {
			logerr("error: missing `cmd' field on line %d\n",
			       y + 1);
			freecte(cte, 5);
			r = -1;
			break;
		}
		cte->cmd = estrdup(col);

		TAILQ_INSERT_TAIL(&ctabhead, cte, entry);
	}

	if (r < 0)
		unloadentries();

	free(line);
	fclose(fp);

	return r;
}

static void
reloadentries(void)
{
	unloadentries();
	if (loadentries() < 0)
		logwarn("warning: discarding old crontab entries\n");
}

static void
sighandler(int sig)
{
	switch (sig) {
	case SIGCHLD:
		chldreap = 1;
		break;
	case SIGHUP:
		reload = 1;
		break;
	case SIGTERM:
		quit = 1;
		break;
	}
}

static void
usage(void)
{
	eprintf("usage: %s [-f file] [-n]\n", argv0);
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	struct ctabentry *cte;
	time_t t;
	struct tm *tm;
	struct sigaction sa;

	ARGBEGIN {
	case 'n':
		nflag = 1;
		break;
	case 'f':
		config = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND

	if (argc > 0)
		usage();

	if (nflag == 0) {
		openlog(argv[0], LOG_CONS | LOG_PID, LOG_CRON);
		if (daemon(1, 0) < 0) {
			logerr("error: failed to daemonize %s\n", strerror(errno));
			return 1;
		}
		if ((fp = fopen(pidfile, "w"))) {
			fprintf(fp, "%d\n", getpid());
			fclose(fp);
		}
	}

	sa.sa_handler = sighandler;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	loadentries();

	while (1) {
		t = time(NULL);
		sleep(60 - t % 60);

		if (quit == 1) {
			if (nflag == 0)
				unlink(pidfile);
			unloadentries();
			/* Don't wait or kill forked processes, just exit */
			break;
		}

		if (reload == 1 || chldreap == 1) {
			if (reload == 1) {
				reloadentries();
				reload = 0;
			}
			if (chldreap == 1) {
				waitjob();
				chldreap = 0;
			}
			continue;
		}

		TAILQ_FOREACH(cte, &ctabhead, entry) {
			t = time(NULL);
			tm = localtime(&t);
			if (matchentry(cte, tm) == 1)
				runjob(cte->cmd);
		}
	}

	if (nflag == 0)
		closelog();

	return 0;
}
