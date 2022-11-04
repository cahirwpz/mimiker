/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include "util.h"

/* because putting integers in pointers is undefined by the standard */
union extra {
	void    *p;
	intmax_t i;
};

/* Argument passed into a primary's function */
struct arg {
	char        *path;
	struct stat *st;
	union extra  extra;
};

/* Information about each primary, for lookup table */
struct pri_info {
	char  *name;
	int    (*func)(struct arg *arg);
	char **(*getarg)(char **argv, union extra *extra);
	void   (*freearg)(union extra extra);
	char   narg; /* -xdev, -depth, -print don't take args but have getarg() */
};

/* Information about operators, for lookup table */
struct op_info {
	char *name;   /* string representation of op           */
	char  type;   /* from tok.type                         */
	char  prec;   /* precedence                            */
	char  nargs;  /* number of arguments (unary or binary) */
	char  lassoc; /* left associative                      */
};

/* Token when lexing/parsing
 * (although also used for the expression tree) */
struct tok {
	struct tok *left, *right; /* if (type == NOT) left = NULL */
	union extra extra;
	union {
		struct pri_info *pinfo; /* if (type == PRIM) */
		struct op_info  *oinfo;
	} u;
	enum {
		PRIM = 0, LPAR, RPAR, NOT, AND, OR, END
	} type;
};

/* structures used for arg.extra.p and tok.extra.p */
struct permarg {
	mode_t mode;
	char   exact;
};

struct okarg {
	char ***braces;
	char **argv;
};

/* for all arguments that take a number
 * +n, n, -n mean > n, == n, < n respectively */
struct narg {
	int (*cmp)(int a, int b);
	int n;
};

struct sizearg {
	struct narg n;
	char bytes; /* size is in bytes, not 512 byte sectors */
};

struct execarg {
	union {
		struct {
			char ***braces; /* NULL terminated list of pointers into argv where {} were */
		} s; /* semicolon */
		struct {
			size_t arglen;  /* number of bytes in argv before files are added */
			size_t filelen; /* numer of bytes in file names added to argv     */
			size_t first;   /* index one past last arg, where first file goes */
			size_t next;    /* index where next file goes                     */
			size_t cap;     /* capacity of argv                               */
		} p; /* plus */
	} u;
	char **argv; /* NULL terminated list of arguments (allocated if isplus) */
	char   isplus; /* -exec + instead of -exec ; */
};

/* used to find loops while recursing through directory structure */
struct findhist {
	struct findhist *next;
	char *path;
	dev_t dev;
	ino_t ino;
};

/* Utility */
static int spawn(char *argv[]);

/* Primaries */
static int pri_name   (struct arg *arg);
static int pri_path   (struct arg *arg);
static int pri_nouser (struct arg *arg);
static int pri_nogroup(struct arg *arg);
static int pri_xdev   (struct arg *arg);
static int pri_prune  (struct arg *arg);
static int pri_perm   (struct arg *arg);
static int pri_type   (struct arg *arg);
static int pri_links  (struct arg *arg);
static int pri_user   (struct arg *arg);
static int pri_group  (struct arg *arg);
static int pri_size   (struct arg *arg);
static int pri_atime  (struct arg *arg);
static int pri_ctime  (struct arg *arg);
static int pri_mtime  (struct arg *arg);
static int pri_exec   (struct arg *arg);
static int pri_ok     (struct arg *arg);
static int pri_print  (struct arg *arg);
static int pri_newer  (struct arg *arg);
static int pri_depth  (struct arg *arg);

/* Getargs */
static char **get_name_arg (char *argv[], union extra *extra);
static char **get_path_arg (char *argv[], union extra *extra);
static char **get_xdev_arg (char *argv[], union extra *extra);
static char **get_perm_arg (char *argv[], union extra *extra);
static char **get_type_arg (char *argv[], union extra *extra);
static char **get_n_arg    (char *argv[], union extra *extra);
static char **get_user_arg (char *argv[], union extra *extra);
static char **get_group_arg(char *argv[], union extra *extra);
static char **get_size_arg (char *argv[], union extra *extra);
static char **get_exec_arg (char *argv[], union extra *extra);
static char **get_ok_arg   (char *argv[], union extra *extra);
static char **get_print_arg(char *argv[], union extra *extra);
static char **get_newer_arg(char *argv[], union extra *extra);
static char **get_depth_arg(char *argv[], union extra *extra);

/* Freeargs */
static void free_extra   (union extra extra);
static void free_exec_arg(union extra extra);
static void free_ok_arg  (union extra extra);

/* Parsing/Building/Running */
static void fill_narg(char *s, struct narg *n);
static struct pri_info *find_primary(char *name);
static struct op_info *find_op(char *name);
static void parse(int argc, char **argv);
static int eval(struct tok *tok, struct arg *arg);
static void find(char *path, struct findhist *hist);
static void usage(void);

/* for comparisons with narg */
static int cmp_gt(int a, int b) { return a >  b; }
static int cmp_eq(int a, int b) { return a == b; }
static int cmp_lt(int a, int b) { return a <  b; }

/* order from find(1p), may want to alphabetize */
static struct pri_info primaries[] = {
	{ "-name"   , pri_name   , get_name_arg , NULL         , 1 },
	{ "-path"   , pri_path   , get_path_arg , NULL         , 1 },
	{ "-nouser" , pri_nouser , NULL         , NULL         , 1 },
	{ "-nogroup", pri_nogroup, NULL         , NULL         , 1 },
	{ "-xdev"   , pri_xdev   , get_xdev_arg , NULL         , 0 },
	{ "-prune"  , pri_prune  , NULL         , NULL         , 1 },
	{ "-perm"   , pri_perm   , get_perm_arg , free_extra   , 1 },
	{ "-type"   , pri_type   , get_type_arg , NULL         , 1 },
	{ "-links"  , pri_links  , get_n_arg    , free_extra   , 1 },
	{ "-user"   , pri_user   , get_user_arg , NULL         , 1 },
	{ "-group"  , pri_group  , get_group_arg, NULL         , 1 },
	{ "-size"   , pri_size   , get_size_arg , free_extra   , 1 },
	{ "-atime"  , pri_atime  , get_n_arg    , free_extra   , 1 },
	{ "-ctime"  , pri_ctime  , get_n_arg    , free_extra   , 1 },
	{ "-mtime"  , pri_mtime  , get_n_arg    , free_extra   , 1 },
	{ "-exec"   , pri_exec   , get_exec_arg , free_exec_arg, 1 },
	{ "-ok"     , pri_ok     , get_ok_arg   , free_ok_arg  , 1 },
	{ "-print"  , pri_print  , get_print_arg, NULL         , 0 },
	{ "-newer"  , pri_newer  , get_newer_arg, NULL         , 1 },
	{ "-depth"  , pri_depth  , get_depth_arg, NULL         , 0 },

	{ NULL, NULL, NULL, NULL, 0 }
};

static struct op_info ops[] = {
	{ "(" , LPAR, 0, 0, 0 }, /* parens are handled specially */
	{ ")" , RPAR, 0, 0, 0 },
	{ "!" , NOT , 3, 1, 0 },
	{ "-a", AND , 2, 2, 1 },
	{ "-o", OR  , 1, 2, 1 },

	{ NULL, 0, 0, 0, 0 }
};

extern char **environ;

static struct tok *toks; /* holds allocated array of all toks created while parsing */
static struct tok *root; /* points to root of expression tree, inside toks array */

static struct timespec start; /* time find was started, used for -[acm]time */

static size_t envlen; /* number of bytes in environ, used to calculate against ARG_MAX */
static size_t argmax; /* value of ARG_MAX retrieved using sysconf(3p) */

static struct {
	char ret  ; /* return value from main                             */
	char depth; /* -depth, directory contents before directory itself */
	char h    ; /* -H, follow symlinks on command line                */
	char l    ; /* -L, follow all symlinks (command line and search)  */
	char prune; /* hit -prune                                         */
	char xdev ; /* -xdev, prune directories on different devices      */
	char print; /* whether we will need -print when parsing           */
} gflags;

/*
 * Utility
 */
static int
spawn(char *argv[])
{
	pid_t pid;
	int status;

	/* flush stdout so that -print output always appears before
	 * any output from the command and does not get cut-off in
	 * the middle of a line. */
	fflush(stdout);

	switch((pid = fork())) {
	case -1:
		eprintf("fork:");
	case 0:
		execvp(*argv, argv);
		weprintf("exec %s failed:", *argv);
		_exit(1);
	}

	/* FIXME: proper course of action for waitpid() on EINTR? */
	waitpid(pid, &status, 0);
	return status;
}

/*
 * Primaries
 */
static int
pri_name(struct arg *arg)
{
	int ret;
	char *path;

	path = estrdup(arg->path);
	ret = !fnmatch((char *)arg->extra.p, basename(path), 0);
	free(path);

	return ret;
}

static int
pri_path(struct arg *arg)
{
	return !fnmatch((char *)arg->extra.p, arg->path, 0);
}

/* FIXME: what about errors? find(1p) literally just says
 * "for which the getpwuid() function ... returns NULL" */
static int
pri_nouser(struct arg *arg)
{
	return !getpwuid(arg->st->st_uid);
}

static int
pri_nogroup(struct arg *arg)
{
	return !getgrgid(arg->st->st_gid);
}

static int
pri_xdev(struct arg *arg)
{
	return 1;
}

static int
pri_prune(struct arg *arg)
{
	return gflags.prune = 1;
}

static int
pri_perm(struct arg *arg)
{
	struct permarg *p = (struct permarg *)arg->extra.p;

	return (arg->st->st_mode & 07777 & (p->exact ? -1U : p->mode)) == p->mode;
}

static int
pri_type(struct arg *arg)
{
	switch ((char)arg->extra.i) {
	default : return 0; /* impossible, but placate warnings */
	case 'b': return S_ISBLK (arg->st->st_mode);
	case 'c': return S_ISCHR (arg->st->st_mode);
	case 'd': return S_ISDIR (arg->st->st_mode);
	case 'l': return S_ISLNK (arg->st->st_mode);
	case 'p': return S_ISFIFO(arg->st->st_mode);
	case 'f': return S_ISREG (arg->st->st_mode);
	case 's': return S_ISSOCK(arg->st->st_mode);
	}
}

static int
pri_links(struct arg *arg)
{
	struct narg *n = arg->extra.p;
	return n->cmp(arg->st->st_nlink, n->n);
}

static int
pri_user(struct arg *arg)
{
	return arg->st->st_uid == (uid_t)arg->extra.i;
}

static int
pri_group(struct arg *arg)
{
	return arg->st->st_gid == (gid_t)arg->extra.i;
}

static int
pri_size(struct arg *arg)
{
	struct sizearg *s = arg->extra.p;
	off_t size = arg->st->st_size;

	if (!s->bytes)
		size = size / 512 + !!(size % 512);

	return s->n.cmp(size, s->n.n);
}

/* FIXME: ignoring nanoseconds in atime, ctime, mtime */
static int
pri_atime(struct arg *arg)
{
	struct narg *n = arg->extra.p;
	return n->cmp((start.tv_sec - arg->st->st_atime) / 86400, n->n);
}

static int
pri_ctime(struct arg *arg)
{
	struct narg *n = arg->extra.p;
	return n->cmp((start.tv_sec - arg->st->st_ctime) / 86400, n->n);
}

static int
pri_mtime(struct arg *arg)
{
	struct narg *n = arg->extra.p;
	return n->cmp((start.tv_sec - arg->st->st_mtime) / 86400, n->n);
}

static int
pri_exec(struct arg *arg)
{
	int status;
	size_t len;
	char **sp, ***brace;
	struct execarg *e = arg->extra.p;

	if (e->isplus) {
		len = strlen(arg->path) + 1;

		/* if we reached ARG_MAX, fork, exec, wait, free file names, reset list */
		if (len + e->u.p.arglen + e->u.p.filelen + envlen > argmax) {
			e->argv[e->u.p.next] = NULL;

			status = spawn(e->argv);
			gflags.ret = gflags.ret || status;

			for (sp = e->argv + e->u.p.first; *sp; sp++)
				free(*sp);

			e->u.p.next = e->u.p.first;
			e->u.p.filelen = 0;
		}

		/* if we have too many files, realloc (with space for NULL termination) */
		if (e->u.p.next + 1 == e->u.p.cap)
			e->argv = ereallocarray(e->argv, e->u.p.cap *= 2, sizeof(*e->argv));

		e->argv[e->u.p.next++] = estrdup(arg->path);
		e->u.p.filelen += len + sizeof(arg->path);

		return 1;
	} else {
		/* insert path everywhere user gave us {} */
		for (brace = e->u.s.braces; *brace; brace++)
			**brace = arg->path;

		status = spawn(e->argv);
		return !!status;
	}
}

static int
pri_ok(struct arg *arg)
{
	int status, reply;
	char ***brace, c;
	struct okarg *o = arg->extra.p;

	fprintf(stderr, "%s: %s ? ", *o->argv, arg->path);
	reply = fgetc(stdin);

	/* throw away rest of line */
	while ((c = fgetc(stdin)) != '\n' && c != EOF)
		/* FIXME: what if the first character of the rest of the line is a null
		 * byte? */
		;

	if (feof(stdin)) /* FIXME: ferror()? */
		clearerr(stdin);

	if (reply != 'y' && reply != 'Y')
		return 0;

	/* insert filename everywhere user gave us {} */
	for (brace = o->braces; *brace; brace++)
		**brace = arg->path;

	status = spawn(o->argv);
	return !!status;
}

static int
pri_print(struct arg *arg)
{
	if (puts(arg->path) == EOF)
		eprintf("puts failed:");
	return 1;
}

/* FIXME: ignoring nanoseconds */
static int
pri_newer(struct arg *arg)
{
	return arg->st->st_mtime > (time_t)arg->extra.i;
}

static int
pri_depth(struct arg *arg)
{
	return 1;
}

/*
 * Getargs
 * consume any arguments for given primary and fill extra
 * return pointer to last argument, the pointer will be incremented in parse()
 */
static char **
get_name_arg(char *argv[], union extra *extra)
{
	extra->p = *argv;
	return argv;
}

static char **
get_path_arg(char *argv[], union extra *extra)
{
	extra->p = *argv;
	return argv;
}

static char **
get_xdev_arg(char *argv[], union extra *extra)
{
	gflags.xdev = 1;
	return argv;
}

static char **
get_perm_arg(char *argv[], union extra *extra)
{
	struct permarg *p = extra->p = emalloc(sizeof(*p));

	if (**argv == '-')
		(*argv)++;
	else
		p->exact = 1;

	p->mode = parsemode(*argv, 0, 0);

	return argv;
}

static char **
get_type_arg(char *argv[], union extra *extra)
{
	if (!strchr("bcdlpfs", **argv))
		eprintf("invalid type %c for -type primary\n", **argv);

	extra->i = **argv;
	return argv;
}

static char **
get_n_arg(char *argv[], union extra *extra)
{
	struct narg *n = extra->p = emalloc(sizeof(*n));
	fill_narg(*argv, n);
	return argv;
}

static char **
get_user_arg(char *argv[], union extra *extra)
{
	char *end;
	struct passwd *p = getpwnam(*argv);

	if (p) {
		extra->i = p->pw_uid;
	} else {
		extra->i = strtol(*argv, &end, 10);
		if (end == *argv || *end)
			eprintf("unknown user '%s'\n", *argv);
	}
	return argv;
}

static char **
get_group_arg(char *argv[], union extra *extra)
{
	char *end;
	struct group *g = getgrnam(*argv);

	if (g) {
		extra->i = g->gr_gid;
	} else {
		extra->i = strtol(*argv, &end, 10);
		if (end == *argv || *end)
			eprintf("unknown group '%s'\n", *argv);
	}
	return argv;
}

static char **
get_size_arg(char *argv[], union extra *extra)
{
	char *p = *argv + strlen(*argv);
	struct sizearg *s = extra->p = emalloc(sizeof(*s));
	/* if the number is followed by 'c', the size will by in bytes */
	if ((s->bytes = (p > *argv && *--p == 'c')))
		*p = '\0';

	fill_narg(*argv, &s->n);
	return argv;
}

static char **
get_exec_arg(char *argv[], union extra *extra)
{
	char **arg, **new, ***braces;
	int nbraces = 0;
	struct execarg *e = extra->p = emalloc(sizeof(*e));

	for (arg = argv; *arg; arg++)
		if (!strcmp(*arg, ";"))
			break;
		else if (arg > argv && !strcmp(*(arg - 1), "{}") && !strcmp(*arg, "+"))
			break;
		else if (!strcmp(*arg, "{}"))
			nbraces++;

	if (!*arg)
		eprintf("no terminating ; or {} + for -exec primary\n");

	e->isplus = **arg == '+';
	*arg = NULL;

	if (e->isplus) {
		*(arg - 1) = NULL; /* don't need the {} in there now */
		e->u.p.arglen = e->u.p.filelen = 0;
		e->u.p.first = e->u.p.next = arg - argv - 1;
		e->u.p.cap = (arg - argv) * 2;
		e->argv = ereallocarray(NULL, e->u.p.cap, sizeof(*e->argv));

		for (arg = argv, new = e->argv; *arg; arg++, new++) {
			*new = *arg;
			e->u.p.arglen += strlen(*arg) + 1 + sizeof(*arg);
		}
		arg++; /* due to our extra NULL */
	} else {
		e->argv = argv;
		e->u.s.braces = ereallocarray(NULL, ++nbraces, sizeof(*e->u.s.braces)); /* ++ for NULL */

		for (arg = argv, braces = e->u.s.braces; *arg; arg++)
			if (!strcmp(*arg, "{}"))
				*braces++ = arg;
		*braces = NULL;
	}
	gflags.print = 0;
	return arg;
}

static char **
get_ok_arg(char *argv[], union extra *extra)
{
	char **arg, ***braces;
	int nbraces = 0;
	struct okarg *o = extra->p = emalloc(sizeof(*o));

	for (arg = argv; *arg; arg++)
		if (!strcmp(*arg, ";"))
			break;
		else if (!strcmp(*arg, "{}"))
			nbraces++;

	if (!*arg)
		eprintf("no terminating ; for -ok primary\n");
	*arg = NULL;

	o->argv = argv;
	o->braces = ereallocarray(NULL, ++nbraces, sizeof(*o->braces));

	for (arg = argv, braces = o->braces; *arg; arg++)
		if (!strcmp(*arg, "{}"))
			*braces++ = arg;
	*braces = NULL;

	gflags.print = 0;
	return arg;
}

static char **
get_print_arg(char *argv[], union extra *extra)
{
	gflags.print = 0;
	return argv;
}

/* FIXME: ignoring nanoseconds */
static char **
get_newer_arg(char *argv[], union extra *extra)
{
	struct stat st;

	if (stat(*argv, &st))
		eprintf("failed to stat '%s':", *argv);

	extra->i = st.st_mtime;
	return argv;
}

static char **
get_depth_arg(char *argv[], union extra *extra)
{
	gflags.depth = 1;
	return argv;
}

/*
 * Freeargs
 */
static void
free_extra(union extra extra)
{
	free(extra.p);
}

static void
free_exec_arg(union extra extra)
{
	int status;
	char **arg;
	struct execarg *e = extra.p;

	if (!e->isplus) {
		free(e->u.s.braces);
	} else {
		e->argv[e->u.p.next] = NULL;

		/* if we have files, do the last exec */
		if (e->u.p.first != e->u.p.next) {
			status = spawn(e->argv);
			gflags.ret = gflags.ret || status;
		}
		for (arg = e->argv + e->u.p.first; *arg; arg++)
			free(*arg);
		free(e->argv);
	}
	free(e);
}

static void
free_ok_arg(union extra extra)
{
	struct okarg *o = extra.p;

	free(o->braces);
	free(o);
}

/*
 * Parsing/Building/Running
 */
static void
fill_narg(char *s, struct narg *n)
{
	char *end;

	switch (*s) {
	case '+': n->cmp = cmp_gt; s++; break;
	case '-': n->cmp = cmp_lt; s++; break;
	default : n->cmp = cmp_eq;      break;
	}
	n->n = strtol(s, &end, 10);
	if (end == s || *end)
		eprintf("bad number '%s'\n", s);
}

static struct pri_info *
find_primary(char *name)
{
	struct pri_info *p;

	for (p = primaries; p->name; p++)
		if (!strcmp(name, p->name))
			return p;
	return NULL;
}

static struct op_info *
find_op(char *name)
{
	struct op_info *o;

	for (o = ops; o->name; o++)
		if (!strcmp(name, o->name))
			return o;
	return NULL;
}

/* given the expression from the command line
 * 1) convert arguments from strings to tok and place in an array duplicating
 *    the infix expression given, inserting "-a" where it was omitted
 * 2) allocate an array to hold the correct number of tok, and convert from
 *    infix to rpn (using shunting-yard), add -a and -print if necessary
 * 3) evaluate the rpn filling in left and right pointers to create an
 *    expression tree (tok are still all contained in the rpn array, just
 *    pointing at eachother)
 */
static void
parse(int argc, char **argv)
{
	struct tok *tok, *rpn, *out, **top, *infix, **stack;
	struct op_info *op;
	struct pri_info *pri;
	char **arg;
	int lasttype = -1;
	size_t ntok = 0;
	struct tok and = { .u.oinfo = find_op("-a"), .type = AND };

	gflags.print = 1;

	/* convert argv to infix expression of tok, inserting in *tok */
	infix = ereallocarray(NULL, 2 * argc + 1, sizeof(*infix));
	for (arg = argv, tok = infix; *arg; arg++, tok++) {
		pri = find_primary(*arg);

		if (pri) { /* token is a primary, fill out tok and get arguments */
			if (lasttype == PRIM || lasttype == RPAR) {
				*tok++ = and;
				ntok++;
			}
			if (pri->getarg) {
				if (pri->narg && !*++arg)
					eprintf("no argument for primary %s\n", pri->name);
				arg = pri->getarg(arg, &tok->extra);
			}
			tok->u.pinfo = pri;
			tok->type = PRIM;
		} else if ((op = find_op(*arg))) { /* token is an operator */
			if (lasttype == LPAR && op->type == RPAR)
				eprintf("empty parens\n");
			if ((lasttype == PRIM || lasttype == RPAR) &&
			    (op->type == NOT || op->type == LPAR)) { /* need another implicit -a */
				*tok++ = and;
				ntok++;
			}
			tok->type = op->type;
			tok->u.oinfo = op;
		} else { /* token is neither primary nor operator, must be path in the wrong place */
			eprintf("paths must precede expression: %s\n", *arg);
		}
		if (tok->type != LPAR && tok->type != RPAR)
			ntok++; /* won't have parens in rpn */
		lasttype = tok->type;
	}
	tok->type = END;
	ntok++;

	if (gflags.print && (arg != argv)) /* need to add -a -print (not just -print) */
		gflags.print++;

	/* use shunting-yard to convert from infix to rpn
	 * https://en.wikipedia.org/wiki/Shunting-yard_algorithm
	 * read from infix, resulting rpn ends up in rpn, next position in rpn is out
	 * push operators onto stack, next position in stack is top */
	rpn = ereallocarray(NULL, ntok + gflags.print, sizeof(*rpn));
	stack = ereallocarray(NULL, argc + gflags.print, sizeof(*stack));
	for (tok = infix, out = rpn, top = stack; tok->type != END; tok++) {
		switch (tok->type) {
		case PRIM: *out++ = *tok; break;
		case LPAR: *top++ =  tok; break;
		case RPAR:
			while (top-- > stack && (*top)->type != LPAR)
				*out++ = **top;
			if (top < stack)
				eprintf("extra )\n");
			break;
		default:
			/* this expression can be simplified, but I decided copy the
			 * verbage from the wikipedia page in order to more clearly explain
			 * what's going on */
			while (top-- > stack &&
			       (( tok->u.oinfo->lassoc && tok->u.oinfo->prec <= (*top)->u.oinfo->prec) ||
			        (!tok->u.oinfo->lassoc && tok->u.oinfo->prec <  (*top)->u.oinfo->prec)))
				*out++ = **top;

			/* top now points to either an operator we didn't pop, or stack[-1]
			 * either way we need to increment it before using it, then
			 * increment again so the stack works */
			top++;
			*top++ = tok;
			break;
		}
	}
	while (top-- > stack) {
		if ((*top)->type == LPAR)
			eprintf("extra (\n");
		*out++ = **top;
	}

	/* if there was no expression, use -print
	 * if there was an expression but no -print, -exec, or -ok, add -a -print
	 * in rpn, not infix */
	if (gflags.print)
		*out++ = (struct tok){ .u.pinfo = find_primary("-print"), .type = PRIM };
	if (gflags.print == 2)
		*out++ = and;

	out->type = END;

	/* rpn now holds all operators and arguments in reverse polish notation
	 * values are pushed onto stack, operators pop values off stack into left
	 * and right pointers, pushing operator node back onto stack
	 * could probably just do this during shunting-yard, but this is simpler
	 * code IMO */
	for (tok = rpn, top = stack; tok->type != END; tok++) {
		if (tok->type == PRIM) {
			*top++ = tok;
		} else {
			if (top - stack < tok->u.oinfo->nargs)
				eprintf("insufficient arguments for operator %s\n", tok->u.oinfo->name);
			tok->right = *--top;
			tok->left  = tok->u.oinfo->nargs == 2 ? *--top : NULL;
			*top++ = tok;
		}
	}
	if (--top != stack)
		eprintf("extra arguments\n");

	toks = rpn;
	root = *top;

	free(infix);
	free(stack);
}

/* for a primary, run and return result
 * for an operator evaluate the left side of the tree, decide whether or not to
 * evaluate the right based on the short-circuit boolean logic, return result
 * NOTE: operator NOT has NULL left side, expression on right side
 */
static int
eval(struct tok *tok, struct arg *arg)
{
	int ret;

	if (!tok)
		return 0;

	if (tok->type == PRIM) {
		arg->extra = tok->extra;
		return tok->u.pinfo->func(arg);
	}

	ret = eval(tok->left, arg);

	if ((tok->type == AND && ret) || (tok->type == OR && !ret) || tok->type == NOT)
		ret = eval(tok->right, arg);

	return ret ^ (tok->type == NOT);
}

/* evaluate path, if it's a directory iterate through directory entries and
 * recurse
 */
static void
find(char *path, struct findhist *hist)
{
	struct stat st;
	DIR *dir;
	struct dirent *de;
	struct findhist *f, cur;
	size_t namelen, pathcap = 0, len;
	struct arg arg = { path, &st, { NULL } };
	char *p, *pathbuf = NULL;

	len = strlen(path) + 2; /* \0 and '/' */

	if ((gflags.l || (gflags.h && !hist) ? stat(path, &st) : lstat(path, &st)) < 0) {
		weprintf("failed to stat %s:", path);
		return;
	}

	gflags.prune = 0;

	/* don't eval now iff we will hit the eval at the bottom which means
	 * 1. we are a directory 2. we have -depth 3. we don't have -xdev or we are
	 * on same device (so most of the time we eval here) */
	if (!S_ISDIR(st.st_mode) ||
	    !gflags.depth        ||
	    (gflags.xdev && hist && st.st_dev != hist->dev))
		eval(root, &arg);

	if (!S_ISDIR(st.st_mode)                          ||
	    gflags.prune                                  ||
	    (gflags.xdev && hist && st.st_dev != hist->dev))
		return;

	for (f = hist; f; f = f->next) {
		if (f->dev == st.st_dev && f->ino == st.st_ino) {
			weprintf("loop detected '%s' is '%s'\n", path, f->path);
			return;
		}
	}
	cur.next = hist;
	cur.path = path;
	cur.dev  = st.st_dev;
	cur.ino  = st.st_ino;

	if (!(dir = opendir(path))) {
		weprintf("failed to opendir %s:", path);
		/* should we just ignore this since we hit an error? */
		if (gflags.depth)
			eval(root, &arg);
		return;
	}

	while (errno = 0, (de = readdir(dir))) {
		if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
			continue;
		namelen = strlen(de->d_name);
		if (len + namelen > pathcap) {
			pathcap = len + namelen;
			pathbuf = erealloc(pathbuf, pathcap);
		}
		p = pathbuf + estrlcpy(pathbuf, path, pathcap);
		if (*--p != '/')
			estrlcat(pathbuf, "/", pathcap);
		estrlcat(pathbuf, de->d_name, pathcap);
		find(pathbuf, &cur);
	}
	free(pathbuf);
	if (errno) {
		weprintf("readdir %s:", path);
		closedir(dir);
		return;
	}
	closedir(dir);

	if (gflags.depth)
		eval(root, &arg);
}

static void
usage(void)
{
	eprintf("usage: %s [-H | -L] path ... [expression ...]\n", argv0);
}

int
main(int argc, char **argv)
{
	char **paths;
	int npaths;
	struct tok *t;

	ARGBEGIN {
	case 'H':
		gflags.h = 1;
		gflags.l = 0;
		break;
	case 'L':
		gflags.l = 1;
		gflags.h = 0;
		break;
	default:
		usage();
	} ARGEND

	paths = argv;

	for (; *argv && **argv != '-' && strcmp(*argv, "!") && strcmp(*argv, "("); argv++)
		;

	if (!(npaths = argv - paths))
		eprintf("must specify a path\n");

	parse(argc - npaths, argv);

	/* calculate number of bytes in environ for -exec {} + ARG_MAX avoidance
	 * libc implementation defined whether null bytes, pointers, and alignment
	 * are counted, so count them */
	for (argv = environ; *argv; argv++)
		envlen += strlen(*argv) + 1 + sizeof(*argv);

	if ((argmax = sysconf(_SC_ARG_MAX)) == (size_t)-1)
		argmax = _POSIX_ARG_MAX;

	if (clock_gettime(CLOCK_REALTIME, &start) < 0)
		weprintf("clock_gettime() failed:");

	while (npaths--)
		find(*paths++, NULL);

	for (t = toks; t->type != END; t++)
		if (t->type == PRIM && t->u.pinfo->freearg)
			t->u.pinfo->freearg(t->extra);
	free(toks);

	gflags.ret |= fshut(stdin, "<stdin>") | fshut(stdout, "<stdout>");

	return gflags.ret;
}
