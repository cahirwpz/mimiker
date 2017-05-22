#ifndef _SYS_VFS_H_
#define _SYS_VFS_H_

typedef struct uio uio_t;
typedef struct thread thread_t;
typedef struct vattr vattr_t;
typedef struct vnode vnode_t;
typedef struct dirent dirent_t;

/* Kernel interface */
int do_open(thread_t *td, char *pathname, int flags, int mode, int *fd);
int do_close(thread_t *td, int fd);
int do_read(thread_t *td, int fd, uio_t *uio);
int do_write(thread_t *td, int fd, uio_t *uio);
int do_lseek(thread_t *td, int fd, off_t offset, int whence);
int do_fstat(thread_t *td, int fd, vattr_t *buf);
int do_dup(thread_t *td, int old);
int do_dup2(thread_t *td, int old, int new);
/* Mount a new instance of the filesystem named fs at the requested path. */
int do_mount(thread_t *td, const char *fs, const char *path);
int do_getdirentries(thread_t *td, int fd, uio_t *uio, off_t *basep);

typedef struct readdir_ops {
  void *(*first)(vnode_t *v);       /* return ptr to first directory entry */
  void *(*next)(void *entry);       /* take next directory entry */
  size_t (*namlen_of)(void *entry); /* filename size (to calc. dirent size) */
  void (*convert)(void *entry, dirent_t *dir); /* make dirent based on entry */
} readdir_ops_t;

int readdir_generic(vnode_t *v, uio_t *uio, readdir_ops_t *ops);

#endif /* !_SYS_VFS_H_ */
