/* See LICENSE file for copyright and license details. */
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#ifndef major
#include <sys/sysmacros.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fs.h"
#include "util.h"

#define BLKSIZ 512

enum Type {
	REG       = '0',
	AREG      = '\0',
	HARDLINK  = '1',
	SYMLINK   = '2',
	CHARDEV   = '3',
	BLOCKDEV  = '4',
	DIRECTORY = '5',
	FIFO      = '6',
	RESERVED  = '7'
};

struct header {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char type;
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char major[8];
	char minor[8];
	char prefix[155];
};

static struct dirtime {
	char *name;
	time_t mtime;
} *dirtimes;

static size_t dirtimeslen;

static int tarfd;
static ino_t tarinode;
static dev_t tardev;

static int mflag, vflag;
static int filtermode;
static const char *filtertool;

static const char *filtertools[] = {
	['J'] = "xz",
	['Z'] = "compress",
	['a'] = "lzma",
	['j'] = "bzip2",
	['z'] = "gzip",
};

static void
pushdirtime(char *name, time_t mtime)
{
	dirtimes = ereallocarray(dirtimes, dirtimeslen + 1, sizeof(*dirtimes));
	dirtimes[dirtimeslen].name = estrdup(name);
	dirtimes[dirtimeslen].mtime = mtime;
	dirtimeslen++;
}

static struct dirtime *
popdirtime(void)
{
	if (dirtimeslen) {
		dirtimeslen--;
		return &dirtimes[dirtimeslen];
	}
	return NULL;
}

static int
comp(int fd, const char *tool, const char *flags)
{
	int fds[2];

	if (pipe(fds) < 0)
		eprintf("pipe:");

	switch (fork()) {
	case -1:
		eprintf("fork:");
	case 0:
		dup2(fd, 1);
		dup2(fds[0], 0);
		close(fds[0]);
		close(fds[1]);

		execlp(tool, tool, flags, NULL);
		weprintf("execlp %s:", tool);
		_exit(1);
	}
	close(fds[0]);
	return fds[1];
}

static int
decomp(int fd, const char *tool, const char *flags)
{
	int fds[2];

	if (pipe(fds) < 0)
		eprintf("pipe:");

	switch (fork()) {
	case -1:
		eprintf("fork:");
	case 0:
		dup2(fd, 0);
		dup2(fds[1], 1);
		close(fds[0]);
		close(fds[1]);

		execlp(tool, tool, flags, NULL);
		weprintf("execlp %s:", tool);
		_exit(1);
	}
	close(fds[1]);
	return fds[0];
}

static ssize_t
eread(int fd, void *buf, size_t n)
{
	ssize_t r;

again:
	r = read(fd, buf, n);
	if (r < 0) {
		if (errno == EINTR)
			goto again;
		eprintf("read:");
	}
	return r;
}

static ssize_t
ewrite(int fd, const void *buf, size_t n)
{
	ssize_t r;

	if ((r = write(fd, buf, n)) != n)
		eprintf("write:");
	return r;
}

static void
putoctal(char *dst, unsigned num, int size)
{
	if (snprintf(dst, size, "%.*o", size - 1, num) >= size)
		eprintf("snprintf: input number too large\n");
}

static int
archive(const char *path)
{
	char b[BLKSIZ];
	struct group *gr;
	struct header *h;
	struct passwd *pw;
	struct stat st;
	size_t chksum, i;
	ssize_t l, r;
	int fd = -1;

	if (lstat(path, &st) < 0) {
		weprintf("lstat %s:", path);
		return 0;
	} else if (st.st_ino == tarinode && st.st_dev == tardev) {
		weprintf("ignoring %s\n", path);
		return 0;
	}

	pw = getpwuid(st.st_uid);
	gr = getgrgid(st.st_gid);

	h = (struct header *)b;
	memset(b, 0, sizeof(b));
	estrlcpy(h->name,    path,                        sizeof(h->name));
	putoctal(h->mode,    (unsigned)st.st_mode & 0777, sizeof(h->mode));
	putoctal(h->uid,     (unsigned)st.st_uid,         sizeof(h->uid));
	putoctal(h->gid,     (unsigned)st.st_gid,         sizeof(h->gid));
	putoctal(h->size,    0,                           sizeof(h->size));
	putoctal(h->mtime,   (unsigned)st.st_mtime,       sizeof(h->mtime));
	memcpy(  h->magic,   "ustar",                     sizeof(h->magic));
	memcpy(  h->version, "00",                        sizeof(h->version));
	estrlcpy(h->uname,   pw ? pw->pw_name : "",       sizeof(h->uname));
	estrlcpy(h->gname,   gr ? gr->gr_name : "",       sizeof(h->gname));

	if (S_ISREG(st.st_mode)) {
		h->type = REG;
		putoctal(h->size, (unsigned)st.st_size,  sizeof(h->size));
		fd = open(path, O_RDONLY);
		if (fd < 0)
			eprintf("open %s:", path);
	} else if (S_ISDIR(st.st_mode)) {
		h->type = DIRECTORY;
	} else if (S_ISLNK(st.st_mode)) {
		h->type = SYMLINK;
		if ((r = readlink(path, h->linkname, sizeof(h->linkname) - 1)) < 0)
			eprintf("readlink %s:", path);
		h->linkname[r] = '\0';
	} else if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
		h->type = S_ISCHR(st.st_mode) ? CHARDEV : BLOCKDEV;
		putoctal(h->major, (unsigned)major(st.st_dev), sizeof(h->major));
		putoctal(h->minor, (unsigned)minor(st.st_dev), sizeof(h->minor));
	} else if (S_ISFIFO(st.st_mode)) {
		h->type = FIFO;
	}

	memset(h->chksum, ' ', sizeof(h->chksum));
	for (i = 0, chksum = 0; i < sizeof(*h); i++)
		chksum += (unsigned char)b[i];
	putoctal(h->chksum, chksum, sizeof(h->chksum));
	ewrite(tarfd, b, BLKSIZ);

	if (fd != -1) {
		while ((l = eread(fd, b, BLKSIZ)) > 0) {
			if (l < BLKSIZ)
				memset(b + l, 0, BLKSIZ - l);
			ewrite(tarfd, b, BLKSIZ);
		}
		close(fd);
	}

	return 0;
}

static int
unarchive(char *fname, ssize_t l, char b[BLKSIZ])
{
	char lname[101], *tmp, *p;
	long mode, major, minor, type, mtime, uid, gid;
	struct header *h = (struct header *)b;
	int fd = -1;
	struct timespec times[2];

	if (!mflag && ((mtime = strtol(h->mtime, &p, 8)) < 0 || *p != '\0'))
		eprintf("strtol %s: invalid number\n", h->mtime);
	if (remove(fname) < 0 && errno != ENOENT)
		weprintf("remove %s:", fname);

	tmp = estrdup(fname);
	mkdirp(dirname(tmp), 0777, 0777);
	free(tmp);

	switch (h->type) {
	case REG:
	case AREG:
	case RESERVED:
		if ((mode = strtol(h->mode, &p, 8)) < 0 || *p != '\0')
			eprintf("strtol %s: invalid number\n", h->mode);
		fd = open(fname, O_WRONLY | O_TRUNC | O_CREAT, 0600);
		if (fd < 0)
			eprintf("open %s:", fname);
		break;
	case HARDLINK:
	case SYMLINK:
		snprintf(lname, sizeof(lname), "%.*s", (int)sizeof(h->linkname),
		         h->linkname);
		if (((h->type == HARDLINK) ? link : symlink)(lname, fname) < 0)
			eprintf("%s %s -> %s:",
			        (h->type == HARDLINK) ? "link" : "symlink",
				fname, lname);
		break;
	case DIRECTORY:
		if ((mode = strtol(h->mode, &p, 8)) < 0 || *p != '\0')
			eprintf("strtol %s: invalid number\n", h->mode);
		if (mkdir(fname, (mode_t)mode) < 0 && errno != EEXIST)
			eprintf("mkdir %s:", fname);
		pushdirtime(fname, mtime);
		break;
	case CHARDEV:
	case BLOCKDEV:
		if ((mode = strtol(h->mode, &p, 8)) < 0 || *p != '\0')
			eprintf("strtol %s: invalid number\n", h->mode);
		if ((major = strtol(h->major, &p, 8)) < 0 || *p != '\0')
			eprintf("strtol %s: invalid number\n", h->major);
		if ((minor = strtol(h->minor, &p, 8)) < 0 || *p != '\0')
			eprintf("strtol %s: invalid number\n", h->minor);
		type = (h->type == CHARDEV) ? S_IFCHR : S_IFBLK;
		if (mknod(fname, type | mode, makedev(major, minor)) < 0)
			eprintf("mknod %s:", fname);
		break;
	case FIFO:
		if ((mode = strtol(h->mode, &p, 8)) < 0 || *p != '\0')
			eprintf("strtol %s: invalid number\n", h->mode);
		if (mknod(fname, S_IFIFO | mode, 0) < 0)
			eprintf("mknod %s:", fname);
		break;
	default:
		eprintf("unsupported tar-filetype %c\n", h->type);
	}

	if ((uid = strtol(h->uid, &p, 8)) < 0 || *p != '\0')
		eprintf("strtol %s: invalid number\n", h->uid);
	if ((gid = strtol(h->gid, &p, 8)) < 0 || *p != '\0')
		eprintf("strtol %s: invalid number\n", h->gid);

	if (fd != -1) {
		for (; l > 0; l -= BLKSIZ)
			if (eread(tarfd, b, BLKSIZ) > 0)
				ewrite(fd, b, MIN(l, BLKSIZ));
		close(fd);
	}

	if (h->type == HARDLINK)
		return 0;

	times[0].tv_sec = times[1].tv_sec = mtime;
	times[0].tv_nsec = times[1].tv_nsec = 0;
	if (!mflag && utimensat(AT_FDCWD, fname, times, AT_SYMLINK_NOFOLLOW) < 0)
		weprintf("utimensat %s:", fname);
	if (h->type == SYMLINK) {
		if (!getuid() && lchown(fname, uid, gid))
			weprintf("lchown %s:", fname);
	} else {
		if (!getuid() && chown(fname, uid, gid))
			weprintf("chown %s:", fname);
		if (chmod(fname, mode) < 0)
			eprintf("fchmod %s:", fname);
	}

	return 0;
}

static void
skipblk(ssize_t l)
{
	char b[BLKSIZ];

	for (; l > 0; l -= BLKSIZ)
		if (!eread(tarfd, b, BLKSIZ))
			break;
}

static int
print(char *fname, ssize_t l, char b[BLKSIZ])
{
	puts(fname);
	skipblk(l);
	return 0;
}

static void
c(int dirfd, const char *name, struct stat *st, void *data, struct recursor *r)
{
	archive(r->path);
	if (vflag)
		puts(r->path);

	if (S_ISDIR(st->st_mode))
		recurse(dirfd, name, NULL, r);
}

static void
sanitize(struct header *h)
{
	size_t i, j;
	struct {
		char  *f;
		size_t l;
	} fields[] = {
		{ h->mode,   sizeof(h->mode)   },
		{ h->uid,    sizeof(h->uid)    },
		{ h->gid,    sizeof(h->gid)    },
		{ h->size,   sizeof(h->size)   },
		{ h->mtime,  sizeof(h->mtime)  },
		{ h->chksum, sizeof(h->chksum) },
		{ h->major,  sizeof(h->major)  },
		{ h->minor,  sizeof(h->minor)  }
	};

	/* Numeric fields can be terminated with spaces instead of
	 * NULs as per the ustar specification.  Patch all of them to
	 * use NULs so we can perform string operations on them. */
	for (i = 0; i < LEN(fields); i++)
		for (j = 0; j < fields[i].l; j++)
			if (fields[i].f[j] == ' ')
				fields[i].f[j] = '\0';
}

static void
chktar(struct header *h)
{
	char tmp[8], *err, *p = (char *)h;
	const char *reason;
	long s1, s2, i;

	if (h->prefix[0] == '\0' && h->name[0] == '\0') {
		reason = "empty filename";
		goto bad;
	}
	if (h->magic[0] && strncmp("ustar", h->magic, 5)) {
		reason = "not ustar format";
		goto bad;
	}
	memcpy(tmp, h->chksum, sizeof(tmp));
	for (i = 0; i < sizeof(tmp); i++)
		if (tmp[i] == ' ')
			tmp[i] = '\0';
	s1 = strtol(tmp, &err, 8);
	if (s1 < 0 || *err != '\0') {
		reason = "invalid checksum";
		goto bad;
	}
	memset(h->chksum, ' ', sizeof(h->chksum));
	for (i = 0, s2 = 0; i < sizeof(*h); i++)
		s2 += (unsigned char)p[i];
	if (s1 != s2) {
		reason = "incorrect checksum";
		goto bad;
	}
	memcpy(h->chksum, tmp, sizeof(h->chksum));
	return;
bad:
	eprintf("malformed tar archive: %s\n", reason);
}

static void
xt(int argc, char *argv[], int mode)
{
	char b[BLKSIZ], fname[256 + 1], *p;
	struct timespec times[2];
	struct header *h = (struct header *)b;
	struct dirtime *dirtime;
	long size;
	int i, n;
	int (*fn)(char *, ssize_t, char[BLKSIZ]) = (mode == 'x') ? unarchive : print;

	while (eread(tarfd, b, BLKSIZ) > 0 && h->name[0]) {
		chktar(h);
		sanitize(h), n = 0;

		/* small dance around non-null terminated fields */
		if (h->prefix[0])
			n = snprintf(fname, sizeof(fname), "%.*s/",
			             (int)sizeof(h->prefix), h->prefix);
		snprintf(fname + n, sizeof(fname) - n, "%.*s",
		         (int)sizeof(h->name), h->name);

		if ((size = strtol(h->size, &p, 8)) < 0 || *p != '\0')
			eprintf("strtol %s: invalid number\n", h->size);

		if (argc) {
			/* only extract the given files */
			for (i = 0; i < argc; i++)
				if (!strcmp(argv[i], fname))
					break;
			if (i == argc) {
				skipblk(size);
				continue;
			}
		}

		/* ignore global pax header craziness */
		if (h->type == 'g' || h->type == 'x') {
			skipblk(size);
			continue;
		}

		fn(fname, size, b);
		if (vflag && mode != 't')
			puts(fname);
	}

	if (mode == 'x' && !mflag) {
		while ((dirtime = popdirtime())) {
			times[0].tv_sec = times[1].tv_sec = dirtime->mtime;
			times[0].tv_nsec = times[1].tv_nsec = 0;
			if (utimensat(AT_FDCWD, dirtime->name, times, 0) < 0)
				eprintf("utimensat %s:", fname);
			free(dirtime->name);
		}
		free(dirtimes);
		dirtimes = NULL;
	}
}

static void
usage(void)
{
	eprintf("usage: %s [-C dir] [-J | -Z | -a | -j | -z] -x [-m | -t] "
	        "[-f file] [file ...]\n"
	        "       %s [-C dir] [-J | -Z | -a | -j | -z] [-h] -c path ... "
	        "[-f file]\n", argv0, argv0);
}

int
main(int argc, char *argv[])
{
	struct recursor r = { .fn = c, .follow = 'P', .flags = DIRFIRST };
	struct stat st;
	char *file = NULL, *dir = ".", mode = '\0';
	int fd;

	ARGBEGIN {
	case 'x':
	case 'c':
	case 't':
		mode = ARGC();
		break;
	case 'C':
		dir = EARGF(usage());
		break;
	case 'f':
		file = EARGF(usage());
		break;
	case 'm':
		mflag = 1;
		break;
	case 'J':
	case 'Z':
	case 'a':
	case 'j':
	case 'z':
		filtermode = ARGC();
		filtertool = filtertools[filtermode];
		break;
	case 'h':
		r.follow = 'L';
		break;
	case 'v':
		vflag = 1;
		break;
	default:
		usage();
	} ARGEND

	if (!mode)
		usage();
	if (mode == 'c')
		if (!argc)
			usage();

	switch (mode) {
	case 'c':
		tarfd = 1;
		if (file && *file != '-') {
			tarfd = open(file, O_WRONLY | O_TRUNC | O_CREAT, 0644);
			if (tarfd < 0)
				eprintf("open %s:", file);
			if (lstat(file, &st) < 0)
				eprintf("lstat %s:", file);
			tarinode = st.st_ino;
			tardev = st.st_dev;
		}

		if (filtertool)
			tarfd = comp(tarfd, filtertool, "-cf");

		if (chdir(dir) < 0)
			eprintf("chdir %s:", dir);
		for (; *argv; argc--, argv++)
			recurse(AT_FDCWD, *argv, NULL, &r);
		break;
	case 't':
	case 'x':
		tarfd = 0;
		if (file && *file != '-') {
			tarfd = open(file, O_RDONLY);
			if (tarfd < 0)
				eprintf("open %s:", file);
		}

		if (filtertool) {
			fd = tarfd;
			tarfd = decomp(tarfd, filtertool, "-cd");
			close(fd);
		}

		if (chdir(dir) < 0)
			eprintf("chdir %s:", dir);
		xt(argc, argv, mode);
		break;
	}

	return recurse_status;
}
