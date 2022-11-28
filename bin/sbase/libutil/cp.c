/* See LICENSE file for copyright and license details. */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include "../fs.h"
#include "../util.h"

int cp_aflag  = 0;
int cp_fflag  = 0;
int cp_pflag  = 0;
int cp_rflag  = 0;
int cp_vflag  = 0;
int cp_status = 0;
int cp_follow;

int
cp(const char *s1, const char *s2, int depth)
{
	DIR *dp;
	int f1, f2, flags = 0;
	struct dirent *d;
	struct stat st;
	struct timespec times[2];
	ssize_t r;
	char target[PATH_MAX], ns1[PATH_MAX], ns2[PATH_MAX];

	if (cp_follow == 'P' || (cp_follow == 'H' && depth))
		flags |= AT_SYMLINK_NOFOLLOW;

	if (fstatat(AT_FDCWD, s1, &st, flags) < 0) {
		weprintf("stat %s:", s1);
		cp_status = 1;
		return 0;
	}

	if (cp_vflag)
		printf("%s -> %s\n", s1, s2);

	if (S_ISLNK(st.st_mode)) {
		if ((r = readlink(s1, target, sizeof(target) - 1)) >= 0) {
			target[r] = '\0';
			if (cp_fflag && unlink(s2) < 0 && errno != ENOENT) {
				weprintf("unlink %s:", s2);
				cp_status = 1;
				return 0;
			} else if (symlink(target, s2) < 0) {
				weprintf("symlink %s -> %s:", s2, target);
				cp_status = 1;
				return 0;
			}
		}
	} else if (S_ISDIR(st.st_mode)) {
		if (!cp_rflag) {
			weprintf("%s is a directory\n", s1);
			cp_status = 1;
			return 0;
		}
		if (!(dp = opendir(s1))) {
			weprintf("opendir %s:", s1);
			cp_status = 1;
			return 0;
		}
		if (mkdir(s2, st.st_mode) < 0 && errno != EEXIST) {
			weprintf("mkdir %s:", s2);
			cp_status = 1;
			closedir(dp);
			return 0;
		}

		while ((d = readdir(dp))) {
			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
				continue;

			estrlcpy(ns1, s1, sizeof(ns1));
			if (s1[strlen(s1) - 1] != '/')
				estrlcat(ns1, "/", sizeof(ns1));
			estrlcat(ns1, d->d_name, sizeof(ns1));

			estrlcpy(ns2, s2, sizeof(ns2));
			if (s2[strlen(s2) - 1] != '/')
				estrlcat(ns2, "/", sizeof(ns2));
			estrlcat(ns2, d->d_name, sizeof(ns2));

			fnck(ns1, ns2, cp, depth + 1);
		}

		closedir(dp);
	} else if (cp_aflag && (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode) ||
	           S_ISSOCK(st.st_mode) || S_ISFIFO(st.st_mode))) {
		if (cp_fflag && unlink(s2) < 0 && errno != ENOENT) {
			weprintf("unlink %s:", s2);
			cp_status = 1;
			return 0;
		} else if (mknod(s2, st.st_mode, st.st_rdev) < 0) {
			weprintf("mknod %s:", s2);
			cp_status = 1;
			return 0;
		}
	} else {
		if ((f1 = open(s1, O_RDONLY)) < 0) {
			weprintf("open %s:", s1);
			cp_status = 1;
			return 0;
		}
		if ((f2 = creat(s2, st.st_mode)) < 0 && cp_fflag) {
			if (unlink(s2) < 0 && errno != ENOENT) {
				weprintf("unlink %s:", s2);
				cp_status = 1;
				close(f1);
				return 0;
			}
			f2 = creat(s2, st.st_mode);
		}
		if (f2 < 0) {
			weprintf("creat %s:", s2);
			cp_status = 1;
			close(f1);
			return 0;
		}
		if (concat(f1, s1, f2, s2) < 0) {
			cp_status = 1;
			close(f1);
			close(f2);
			return 0;
		}

		close(f1);
		close(f2);
	}

	if (cp_aflag || cp_pflag) {
		/* atime and mtime */
		times[0] = st.st_atim;
		times[1] = st.st_mtim;
		if (utimensat(AT_FDCWD, s2, times, AT_SYMLINK_NOFOLLOW) < 0) {
			weprintf("utimensat %s:", s2);
			cp_status = 1;
		}

		/* owner and mode */
		if (!S_ISLNK(st.st_mode)) {
			if (chown(s2, st.st_uid, st.st_gid) < 0) {
				weprintf("chown %s:", s2);
				cp_status = 1;
				st.st_mode &= ~(S_ISUID | S_ISGID);
			}
			if (chmod(s2, st.st_mode) < 0) {
				weprintf("chmod %s:", s2);
				cp_status = 1;
			}
		} else {
			if (lchown(s2, st.st_uid, st.st_gid) < 0) {
				weprintf("lchown %s:", s2);
				cp_status = 1;
				return 0;
			}
		}
	}

	return 0;
}
