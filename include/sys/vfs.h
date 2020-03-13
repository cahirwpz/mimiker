#ifndef _SYS_VFS_H_
#define _SYS_VFS_H_

#ifdef _KERNEL

typedef struct uio uio_t;
typedef struct proc proc_t;
typedef struct thread thread_t;
typedef struct vnode vnode_t;
typedef struct stat stat_t;
typedef struct statfs statfs_t;
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

/* Path name component flags. Used to represent internal state. */
#define VNR_ISLASTPC 0x00000001   /* this is last component of pathname */
#define VNR_REQUIREDIR 0x00000002 /* must be a directory */

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

#define COMPONENTNAME(str)                                                     \
  (componentname_t) {                                                          \
    .cn_flags = 0, .cn_nameptr = str, .cn_namelen = strlen(str)                \
  }

bool componentname_equal(const componentname_t *cn, const char *name);

/* Kernel interface */
int do_open(proc_t *p, char *pathname, int flags, mode_t mode, int *fd);
int do_close(proc_t *p, int fd);
int do_read(proc_t *p, int fd, uio_t *uio);
int do_write(proc_t *p, int fd, uio_t *uio);
int do_lseek(proc_t *p, int fd, off_t offset, int whence, off_t *newoffp);
int do_fstat(proc_t *p, int fd, stat_t *sb);
int do_dup(proc_t *p, int oldfd, int *newfdp);
int do_dup2(proc_t *p, int oldfd, int newfd);
int do_fcntl(proc_t *p, int fd, int cmd, int arg, int *resp);
int do_unlink(proc_t *p, char *path);
int do_mkdir(proc_t *p, char *path, mode_t mode);
int do_rmdir(proc_t *p, char *path);
int do_ftruncate(proc_t *p, int fd, off_t length);
int do_access(proc_t *p, char *path, int amode);
int do_chmod(proc_t *p, char *path, mode_t mode);
int do_chown(proc_t *p, char *path, int uid, int gid);
int do_utimes(proc_t *p, char *path, timeval_t *tptr);
int do_symlink(proc_t *p, char *path, char *link);
ssize_t do_readlinkat(proc_t *p, int fd, char *path, uio_t *uio);
int do_symlinkat(proc_t *p, char *target, int newdirfd, char *linkpath);
int do_rename(proc_t *p, char *from, char *to);
int do_chdir(proc_t *p, char *path);
int do_fchdir(proc_t *p, int fd);
int do_getcwd(proc_t *p, char *buf, size_t *lastp);
int do_umask(proc_t *p, int newmask, int *oldmaskp);
int do_ioctl(proc_t *p, int fd, u_long cmd, void *data);
int do_truncate(proc_t *p, char *path, off_t length);
int do_ftruncate(proc_t *p, int fd, off_t length);
int do_fstatat(proc_t *p, int fd, char *path, stat_t *sb, int flag);

/* Mount a new instance of the filesystem named fs at the requested path. */
int do_mount(const char *fs, const char *path);
int do_statfs(proc_t *p, char *path, statfs_t *buf);
int do_getdents(proc_t *p, int fd, uio_t *uio);

/* Finds the vnode corresponding to the given path.
 * Increases use count on returned vnode. */
int vfs_namelookupat(const char *path, vnode_t *atdir, uint32_t flags,
                     vnode_t **vp);
#define vfs_namelookup(path, vp) vfs_namelookupat(path, NULL, VNR_FOLLOW, vp)

/* Yield the vnode for an existing entry; or, if there is none, yield NULL.
 * Parent vnode is locked and held; vnode, if exists, is only held.*/
int vfs_namecreateat(const char *path, vnode_t *atdir, uint32_t flags,
                     vnode_t **dvp, vnode_t **vp, componentname_t *cn);
#define vfs_namecreate(path, dvp, vp, cn)                                      \
  vfs_namecreateat(path, NULL, VNR_FOLLOW, dvp, vp, cn)

/* Both vnode and its parent is held and locked. */
int vfs_namedeleteat(const char *path, vnode_t *atdir, uint32_t flags,
                     vnode_t **dvp, vnode_t **vp, componentname_t *cn);
#define vfs_namedelete(path, dvp, vp, cn)                                      \
  vfs_namedeleteat(path, NULL, VNR_FOLLOW, dvp, vp, cn)

/* Uncovers mountpoint if node is mounted */
void vfs_maybe_ascend(vnode_t **vp);

/* Looks up the vnode corresponding to the pathname and opens it into f. */
int vfs_open(file_t *f, char *pathname, int flags, int mode);

/* Finds name of v-node in given directory. */
int vfs_name_in_dir(vnode_t *dv, vnode_t *v, char *buf, size_t *lastp);

#endif /* !_KERNEL */

#endif /* !_SYS_VFS_H_ */
