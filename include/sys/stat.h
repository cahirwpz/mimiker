/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)stat.h	8.12 (Berkeley) 8/17/94
 */

#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

#include <stdint.h>
#include <sys/time.h>

typedef struct stat {
  dev_t st_dev;            /* inode's device */
  ino_t st_ino;            /* inode's number */
  mode_t st_mode;          /* inode protection mode */
  nlink_t st_nlink;        /* number of hard links */
  uid_t st_uid;            /* user ID of the file's owner */
  gid_t st_gid;            /* group ID of the file's group */
  dev_t st_rdev;           /* device type */
  off_t st_size;           /* file size, in bytes */
  struct timespec st_atim; /* time of last access */
  struct timespec st_mtim; /* time of last data modification */
  struct timespec st_ctim; /* time of last file status change */
  blksize_t st_blksize;    /* optimal blocksize for I/O */
  blkcnt_t st_blocks;      /* blocks allocated for file */
} stat_t;

/* Standard-mandated compatibility */
#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
#define st_birthtime st_birthtim.tv_sec

#define st_atimespec st_atim
#define st_atimensec st_atim.tv_nsec
#define st_mtimespec st_mtim
#define st_mtimensec st_mtim.tv_nsec
#define st_ctimespec st_ctim
#define st_ctimensec st_ctim.tv_nsec

#define S_ISUID 0004000 /* set user id on execution */
#define S_ISGID 0002000 /* set group id on execution */
#define S_ISTXT 0001000 /* sticky bit */

#define S_IRWXU 0000700 /* RWX mask for owner */
#define S_IRUSR 0000400 /* R for owner */
#define S_IWUSR 0000200 /* W for owner */
#define S_IXUSR 0000100 /* X for owner */

#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#define S_IEXEC S_IXUSR

#define S_IRWXG 0000070 /* RWX mask for group */
#define S_IRGRP 0000040 /* R for group */
#define S_IWGRP 0000020 /* W for group */
#define S_IXGRP 0000010 /* X for group */

#define S_IRWXO 0000007 /* RWX mask for other */
#define S_IROTH 0000004 /* R for other */
#define S_IWOTH 0000002 /* W for other */
#define S_IXOTH 0000001 /* X for other */

#define S_IFMT 0170000   /* type of file mask */
#define S_IFIFO 0010000  /* named pipe (fifo) */
#define S_IFCHR 0020000  /* character special */
#define S_IFDIR 0040000  /* directory */
#define S_IFBLK 0060000  /* block special */
#define S_IFREG 0100000  /* regular */
#define S_IFLNK 0120000  /* symbolic link */
#define S_ISVTX 0001000  /* save swapped text even after use */
#define S_IFSOCK 0140000 /* socket */

#define S_ISDIR(m) ((m & S_IFMT) == S_IFDIR)   /* directory */
#define S_ISCHR(m) ((m & S_IFMT) == S_IFCHR)   /* char special */
#define S_ISBLK(m) ((m & S_IFMT) == S_IFBLK)   /* block special */
#define S_ISREG(m) ((m & S_IFMT) == S_IFREG)   /* regular file */
#define S_ISFIFO(m) ((m & S_IFMT) == S_IFIFO)  /* fifo */
#define S_ISLNK(m) ((m & S_IFMT) == S_IFLNK)   /* symbolic link */
#define S_ISSOCK(m) ((m & S_IFMT) == S_IFSOCK) /* socket */

#define S_BLKSIZE 512 /* block size used in the stat struct */

#define ACCESSPERMS (S_IRWXU | S_IRWXG | S_IRWXO)
#define DEFFILEMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define ALLPERMS (S_ISUID | S_ISGID | S_ISTXT | S_IRWXU | S_IRWXG | S_IRWXO)

#ifndef _KERNEL
int chmod(const char *, mode_t);
int mkdir(const char *, mode_t);
int mkfifo(const char *, mode_t);
int mknod(const char *, mode_t, dev_t);
int stat(const char *pathname, stat_t *sb);
int fstat(int fd, stat_t *sb);
mode_t umask(mode_t);
int lstat(const char *, struct stat *);
int mkdir(const char *, mode_t);
int fchmod(int, mode_t);
int lchmod(const char *, mode_t);

/*
 * X/Open Extended API set 2 (a.k.a. C063)
 */

int fstatat(int, const char *, struct stat *, int);
int mkdirat(int, const char *, mode_t);
int fchmodat(int, const char *, mode_t, int);

int utimens(const char *, const struct timespec *);
int lutimens(const char *, const struct timespec *);
int futimens(int, const struct timespec *);
#endif /* !_KERNEL */

#endif /* !_SYS_STAT_H_ */
