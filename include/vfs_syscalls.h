#ifndef _SYS_VFS_SYSCALLS_H_
#define _SYS_VFS_SYSCALLS_H_

#include <uio.h>

typedef struct thread thread_t;
typedef struct syscall_args syscall_args_t;

int do_open(thread_t *td, char *pathname, int flags, int mode);
int do_read(int fd, uio_t *uio);
int do_write(int fd, uio_t *uio);

int sys_open(thread_t *td, syscall_args_t *args);
int sys_close(thread_t *td, syscall_args_t *args);
int sys_read(thread_t *td, syscall_args_t *args);
int sys_write(thread_t *td, syscall_args_t *args);

#endif /* !_SYS_VFS_SYSCALLS_H_ */
