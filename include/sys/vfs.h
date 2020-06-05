#ifndef _SYS_VFS_H_
#define _SYS_VFS_H_

#ifdef _KERNEL

typedef struct uio uio_t;
typedef struct proc proc_t;
typedef struct thread thread_t;
typedef struct vnode vnode_t;
typedef struct stat stat_t;
typedef struct statvfs statvfs_t;
typedef struct timeval timeval_t;
typedef struct file file_t;

/*
 * vnr (vfs name resolver) is used to convert pathnames to file system vnodes
 * and is loosely based on NetBSD's namei interface. You can find details in
 * NAMEI(9).
 */

typedef enum {
  VNR_LOOKUP = 0,
  VNR_CREATE = 1,
  VNR_DELETE = 2,
  VNR_RENAME = 3,
} vnrop_t;

/* VNR lookup flags.  */
#define VNR_FOLLOW 0x00000001 /* follow symbolic links */

/*
 * Encapsulation of lookup parameters.
 */
typedef struct componentname {
  uint32_t cn_flags;
  const char *cn_nameptr; /* not NULL-terminated */
  size_t cn_namelen;
} componentname_t;

typedef struct {
  /*
   * Arguments fed into name resolver.
   */
  vnode_t *vs_atdir; /* startup dir, cwd if null */
  vnrop_t vs_op;     /* vnr operation type */
  uint32_t vs_flags; /* flags to vfs name resolver */
  const char *vs_path;

  /*
   * Results returned from name resolver.
   */
  vnode_t *vs_vp;            /* vnode of result */
  vnode_t *vs_dvp;           /* vnode of parent directory */
  componentname_t vs_lastcn; /* last component's name */

  /*
   * Name resolver internal state. DO NOT TOUCH!
   */
  char *vs_pathbuf;  /* pathname buffer */
  size_t vs_pathlen; /* remaining chars in path */
  componentname_t vs_cn;
  const char *vs_nextcn;
  int vs_loopcnt; /* count of symlinks encountered */
} vnrstate_t;

#define COMPONENTNAME(str)                                                     \
  (componentname_t) {                                                          \
    .cn_flags = 0, .cn_nameptr = str, .cn_namelen = strlen(str)                \
  }

bool componentname_equal(const componentname_t *cn, const char *name);

/* Procedures called by system calls implementation. */
int do_open(proc_t *p, char *pathname, int flags, mode_t mode, int *fdp);
int do_openat(proc_t *p, int fdat, char *pathname, int flags, mode_t mode,
              int *fdp);
int do_unlinkat(proc_t *p, int fd, char *path, int flag);
int do_mkdirat(proc_t *p, int fd, char *path, mode_t mode);
int do_ftruncate(proc_t *p, int fd, off_t length);
int do_faccessat(proc_t *p, int fd, char *path, int mode, int flags);
int do_chown(proc_t *p, char *path, int uid, int gid);
int do_utimes(proc_t *p, char *path, timeval_t *tptr);
ssize_t do_readlinkat(proc_t *p, int fd, char *path, uio_t *uio);
int do_symlinkat(proc_t *p, char *target, int newdirfd, char *linkpath);
int do_rename(proc_t *p, char *from, char *to);
int do_chdir(proc_t *p, const char *path);
int do_fchdir(proc_t *p, int fd);
int do_getcwd(proc_t *p, char *buf, size_t *lastp);
int do_truncate(proc_t *p, char *path, off_t length);
int do_ftruncate(proc_t *p, int fd, off_t length);
int do_fstatat(proc_t *p, int fd, char *path, stat_t *sb, int flag);
int do_linkat(proc_t *p, int fd, char *path, int linkfd, char *linkpath,
              int flags);
int do_fchmod(proc_t *p, int fd, mode_t mode);
int do_fchmodat(proc_t *p, int fd, char *path, mode_t mode, int flag);

/* Mount a new instance of the filesystem named fs at the requested path. */
int do_mount(const char *fs, const char *path);
int do_getdents(proc_t *p, int fd, uio_t *uio);
int do_statvfs(proc_t *p, char *path, statvfs_t *buf);
int do_fstatvfs(proc_t *p, int fd, statvfs_t *buf);

/* Initialize & destroy structures required to perform name resolution. */
int vnrstate_init(vnrstate_t *vs, vnrop_t op, uint32_t flags, const char *path);
void vnrstate_destroy(vnrstate_t *vs);

/* Perform name resolution with specified operation. */
int vfs_nameresolve(vnrstate_t *vs);

/* Finds the vnode corresponding to the given path.
 * Increases use count on returned vnode. */
int vfs_namelookup(const char *path, vnode_t **vp);

/* Uncovers mountpoint if node is mounted. */
void vfs_maybe_ascend(vnode_t **vp);

/* Get the root of filesystem if node is a mountpoint. */
int vfs_maybe_descend(vnode_t **vp);

/* Finds name of v-node in given directory. */
int vfs_name_in_dir(vnode_t *dv, vnode_t *v, char *buf, size_t *lastp);

#endif /* !_KERNEL */

#endif /* !_SYS_VFS_H_ */
