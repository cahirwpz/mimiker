/*	$NetBSD: dirent_private.h,v 1.4 2010/09/26 02:26:59 yamt Exp $	*/

/*
 * One struct _dirpos is malloced to describe the current directory
 * position each time telldir is called. It records the current magic
 * cookie returned by getdents and the offset within the buffer associated
 * with that return value.
 */
struct dirpos {
  struct dirpos *dp_next; /* next structure in list */
  off_t dp_seek;          /* magic cookie returned by getdents */
  long dp_loc;            /* offset of entry in buffer */
};

/* structure describing an open directory. */
struct _dirdesc {
  /*
   * dd_fd should be kept intact to preserve ABI compat.  see dirfd().
   */
  int dd_fd; /* file descriptor associated with directory */
  /*
   * the rest is hidden from user.
   */
  long dd_loc;       /* offset in current buffer */
  long dd_size;      /* amount of data returned by getdents */
  char *dd_buf;      /* data buffer */
  int dd_len;        /* size of data buffer */
  off_t dd_seek;     /* magic cookie returned by getdents */
  void *dd_internal; /* state for seekdir/telldir */
  int dd_flags;      /* flags for readdir */
  void *dd_lock;     /* lock for concurrent access */
};

#define dirfd(dirp) ((dirp)->dd_fd)

void _seekdir_unlocked(struct _dirdesc *, long);
long _telldir_unlocked(struct _dirdesc *);
int _initdir(DIR *, int, const char *);
void _finidir(DIR *);
struct dirent;
struct dirent *_readdir_unlocked(struct _dirdesc *, int);

DIR *__opendir2(const char *, int);

/* flags for __opendir2() */
#define DTF_HIDEW 0x0001     /* hide whiteout entries */
#define DTF_NODUP 0x0002     /* don't return duplicate names */
#define DTF_REWIND 0x0004    /* rewind after reading union stack */
#define __DTF_READALL 0x0008 /* everything has been read */
#define __DTF_RETRY_ON_BADCOOKIE                                               \
  0x0001 /* retry on EINVAL                                                    \
         (only valid with __DTF_READALL) */

/* definitions for library routines operating on directories. */
#define DIRBLKSIZ 1024
