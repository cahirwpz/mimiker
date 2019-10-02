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

/* Kernel interface */
int do_open(proc_t *p, char *pathname, int flags, mode_t mode, int *fd);
int do_close(proc_t *p, int fd);
int do_read(proc_t *p, int fd, uio_t *uio);
int do_write(proc_t *p, int fd, uio_t *uio);
int do_lseek(proc_t *p, int fd, off_t offset, int whence, off_t *newoffp);
int do_fstat(proc_t *p, int fd, stat_t *sb);
int do_dup(proc_t *p, int oldfd, int *newfdp);
int do_dup2(proc_t *p, int oldfd, int newfd);
int do_unlink(proc_t *p, char *path);
int do_mkdir(proc_t *p, char *path, mode_t mode);
int do_rmdir(proc_t *p, char *path);
int do_ftruncate(proc_t *p, int fd, off_t length);
int do_access(proc_t *p, char *path, int amode);
int do_chmod(proc_t *p, char *path, mode_t mode);
int do_chown(proc_t *p, char *path, int uid, int gid);
int do_utimes(proc_t *p, char *path, timeval_t *tptr);
int do_stat(proc_t *p, char *path, stat_t *sb);
int do_symlink(proc_t *p, char *path, char *link);
ssize_t do_readlink(proc_t *p, char *path, char *buf, size_t count);
int do_rename(proc_t *p, char *from, char *to);
int do_chdir(proc_t *p, char *path);
char *do_getcwd(proc_t *p, char *buf, size_t size);
int do_umask(proc_t *p, int newmask);

/* Mount a new instance of the filesystem named fs at the requested path. */
int do_mount(const char *fs, const char *path);
int do_statfs(proc_t *p, char *path, statfs_t *buf);
int do_getdirentries(proc_t *p, int fd, uio_t *uio, off_t *basep);

/* Finds the vnode corresponding to the given path.
 * Increases use count on returned vnode. */
int vfs_lookup(const char *pathname, vnode_t **vp);

/* Looks up the vnode corresponding to the pathname and opens it into f. */
int vfs_open(file_t *f, char *pathname, int flags, int mode);

#endif /* !_KERNEL */

#endif /* !_SYS_VFS_H_ */
