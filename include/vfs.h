#ifndef _SYS_VFS_H_
#define _SYS_VFS_H_

typedef struct uio uio_t;
typedef struct thread thread_t;
typedef struct vnode vnode_t;
typedef struct stat stat_t;
typedef struct statfs statfs_t;
typedef struct timeval timeval_t;
typedef struct file file_t;

/* Kernel interface */
int do_open(thread_t *td, char *pathname, int flags, mode_t mode, int *fd);
int do_close(thread_t *td, int fd);
int do_read(thread_t *td, int fd, uio_t *uio);
int do_write(thread_t *td, int fd, uio_t *uio);
int do_lseek(thread_t *td, int fd, off_t offset, int whence);
int do_fstat(thread_t *td, int fd, stat_t *sb);
int do_dup(thread_t *td, int old);
int do_dup2(thread_t *td, int old, int new);
int do_unlink(thread_t *td, char *path);
int do_mkdir(thread_t *td, char *path, mode_t mode);
int do_rmdir(thread_t *td, char *path);
int do_ftruncate(thread_t *td, int fd, off_t length);
int do_access(thread_t *td, char *path, mode_t mode);
int do_chmod(thread_t *td, char *path, mode_t mode);
int do_chown(thread_t *td, char *path, int uid, int gid);
int do_utimes(thread_t *td, char *path, timeval_t *tptr);
int do_stat(thread_t *td, char *path, stat_t *ub);
int do_symlink(thread_t *td, char *path, char *link);
ssize_t do_readlink(thread_t *td, char *path, char *buf, size_t count);
int do_rename(thread_t *td, char *from, char *to);
int do_chdir(thread_t *td, char *path);
char *do_getcwd(thread_t *td, char *buf, size_t size);
int do_umask(thread_t *td, int newmask);

/* Mount a new instance of the filesystem named fs at the requested path. */
int do_mount(thread_t *td, const char *fs, const char *path);
int do_statfs(thread_t *td, char *path, statfs_t *buf);
int do_getdirentries(thread_t *td, int fd, uio_t *uio, off_t *basep);

/* Finds the vnode corresponding to the given path.
 * Increases use count on returned vnode. */
int vfs_lookup(const char *pathname, vnode_t **vp);

/* Looks up the vnode corresponding to the pathname and opens it into f. */
int vfs_open(file_t *f, char *pathname, int flags, int mode);

#endif /* !_SYS_VFS_H_ */
