#ifndef _SYS_FCNTL_H_
#define _SYS_FCNTL_H_

/* Always ensure that these are consistent with <stdio.h> and <unistd.h>! */
#ifndef SEEK_SET
#define SEEK_SET 0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END 2 /* set file offset to EOF plus offset */
#endif

/*
 * File status flags: these are used by open(2), fcntl(2).
 * They are also used (indirectly) in the kernel file structure f_flags,
 * which is a superset of the open/fcntl flags.  Open flags and f_flags
 * are inter-convertible using OFLAGS(fflags) and FFLAGS(oflags).
 * Open/fcntl flags begin with O_; kernel-internal flags begin with F.
 */
/* open-only flags */
#define O_RDONLY 0x00000000  /* open for reading only */
#define O_WRONLY 0x00000001  /* open for writing only */
#define O_RDWR 0x00000002    /* open for reading and writing */
#define O_ACCMODE 0x00000003 /* mask for above modes */

/*
 * Kernel encoding of open mode; separate read and write bits that are
 * independently testable: 1 greater than the above.
 */
#define O_NONBLOCK 0x00000004  /* no delay */
#define O_APPEND 0x00000008    /* set append mode */
#define O_SHLOCK 0x00000010    /* open with shared file lock */
#define O_EXLOCK 0x00000020    /* open with exclusive file lock */
#define O_ASYNC 0x00000040     /* signal pgrp when data ready */
#define O_SYNC 0x00000080      /* synchronous writes */
#define O_NOFOLLOW 0x00000100  /* don't follow symlinks on the last */
                               /* path component */
#define O_CREAT 0x00000200     /* create if nonexistent */
#define O_TRUNC 0x00000400     /* truncate to zero length */
#define O_EXCL 0x00000800      /* error if already exists */
#define O_DIRECTORY 0x00001000 /* fail if not a directory */
#define O_CLOEXEC 0x00002000   /* set close on exec */
#define O_REGULAR 0x00004000   /* fail if not a regular file */
#define O_DIRECT 0x00008000    /* direct I/O hint */
#define O_DSYNC 0x00010000     /* write: I/O data completion */
#define O_RSYNC 0x00020000     /* read: I/O completion as for write */

/* Constants used for fcntl(2) */

/* command values */
#define F_DUPFD 0          /* duplicate file descriptor */
#define F_GETFD 1          /* get file descriptor flags */
#define F_SETFD 2          /* set file descriptor flags */
#define F_GETFL 3          /* get file status flags */
#define F_SETFL 4          /* set file status flags */
#define F_DUPFD_CLOEXEC 12 /* close on exec duplicated fd */

/* file descriptor flags (F_GETFD, F_SETFD) */
#define FD_CLOEXEC 1 /* close-on-exec flag */

/*
 * Constants for X/Open Extended API set 2 (a.k.a. C063)
 */
#define AT_FDCWD -100             /* Use cwd for relative link target */
#define AT_SYMLINK_NOFOLLOW 0x200 /* Do not follow symlinks */
#define AT_SYMLINK_FOLLOW 0x400   /* Follow symbolic link */
/* Remove directory instead of unlinking file. */
#define AT_REMOVEDIR 0x800
/* Test access permitted for effective IDs, not real IDs. */
#define AT_EACCESS 0x1000

#ifndef _KERNEL
#include <sys/types.h>

__BEGIN_DECLS
int open(const char *, int, ...);
int openat(int, const char *, int, ...);
int creat(const char *, mode_t);
int fcntl(int, int, ...);
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_SYS_FCNTL_H_ */
