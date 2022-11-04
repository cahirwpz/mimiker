/* See LICENSE file for copyright and license details. */
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

struct history {
	struct history *prev;
	dev_t dev;
	ino_t ino;
};

struct recursor {
	void (*fn)(int, const char *, struct stat *, void *, struct recursor *);
	char path[PATH_MAX];
	size_t pathlen;
	struct history *hist;
	int depth;
	int maxdepth;
	int follow;
	int flags;
};

enum {
	SAMEDEV  = 1 << 0,
	DIRFIRST = 1 << 1,
	SILENT   = 1 << 2,
};

extern int cp_aflag;
extern int cp_fflag;
extern int cp_pflag;
extern int cp_rflag;
extern int cp_vflag;
extern int cp_follow;
extern int cp_status;

extern int rm_fflag;
extern int rm_rflag;
extern int rm_status;

extern int recurse_status;

void recurse(int, const char *, void *, struct recursor *);

int cp(const char *, const char *, int);
void rm(int, const char *, struct stat *st, void *, struct recursor *);
