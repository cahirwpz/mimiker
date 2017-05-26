#ifndef _SYS_VFS_H_
#define _SYS_VFS_H_

#include <uio.h>

typedef struct thread thread_t;
typedef struct vnode vnode_t;
typedef struct vattr vattr_t;
typedef struct file file_t;

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

/* Finds the vnode corresponding to the given path.
 * Increases use count on returned vnode. */
int vfs_lookup(const char *pathname, vnode_t **vp);

/* Looks up the vnode corresponding to the pathname and opens it into f. */
int vfs_open(file_t *f, char *pathname, int flags, int mode);

#endif /* !_SYS_VFS_H_ */
