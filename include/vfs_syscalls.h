#ifndef _SYS_VFS_SYSCALLS_H_
#define _SYS_VFS_SYSCALLS_H_

#include <uio.h>

typedef struct thread thread_t;
typedef struct syscall_args syscall_args_t;
typedef struct vattr vattr_t;

/* Kernel interface */
int do_open(thread_t *td, char *pathname, int flags, int mode, int *fd);
int do_close(thread_t *td, int fd);
int do_read(thread_t *td, int fd, uio_t *uio);
int do_write(thread_t *td, int fd, uio_t *uio);
int do_lseek(thread_t *td, int fd, off_t offset, int whence);
int do_fstat(thread_t *td, int fd, vattr_t *buf);
int do_dup(thread_t *td, int oldfd);
int do_dup2(thread_t *td, int oldfd, int newfd);

/* Syscall interface */
int sys_open(thread_t *td, syscall_args_t *args);
int sys_close(thread_t *td, syscall_args_t *args);
int sys_read(thread_t *td, syscall_args_t *args);
int sys_write(thread_t *td, syscall_args_t *args);
int sys_lseek(thread_t *td, syscall_args_t *args);
int sys_fstat(thread_t *td, syscall_args_t *args);
int sys_dup(thread_t *td, syscall_args_t *args);
int sys_dup2(thread_t *td, syscall_args_t *args);

#endif /* !_SYS_VFS_SYSCALLS_H_ */
