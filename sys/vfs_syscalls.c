#include <vfs_syscalls.h>
#include <file.h>
#include <filedesc.h>
#include <mount.h>
#include <sysent.h>
#include <systm.h>
#include <stdc.h>
#include <vm_map.h>

int do_open(thread_t *td, char *pathname, int flags, int mode, int *fd) {
  /* Allocate a file structure, but do not install descriptor yet. */
  file_t *f = file_alloc_noinstall();
  /* Try opening file. Fill the file structure. */
  int error = vfs_open(f, pathname, flags, mode);
  if (error)
    goto fail;
  /* Now install the file in descriptor table. */
  error = file_install_desc(td->td_fdt, f, fd);
  if (error)
    goto fail;

  /* The file is stored in the descriptor table, but we got our own reference
     when we asked to allocate the file. Thus we need to release that initial
     ref. */
  file_drop(f);
  return 0;

fail:
  file_drop(f);
  return error;
}

int do_close(thread_t *td, int fd) {
  return file_desc_close(td->td_fdt, fd);
}

int do_read(thread_t *td, int fd, uio_t *uio) {
  file_t *f;
  int res = file_get_read(td, fd, &f);
  if (res)
    return res;
  res = f->f_ops->fo_read(f, td, uio);
  file_drop(f);
  return res;
}

int do_write(thread_t *td, int fd, uio_t *uio) {
  file_t *f;
  int res = file_get_write(td, fd, &f);
  if (res)
    return res;
  res = f->f_ops->fo_write(f, td, uio);
  file_drop(f);
  return res;
}

/* == System calls interface === */

int sys_open(thread_t *td, syscall_args_t *args) {
  char *user_pathname = (char *)args->args[0];
  int flags = args->args[1];
  int mode = args->args[2];

  int error = 0;
  char pathname[256];
  size_t n = 0;

  /* Copyout pathname. */
  error = copyinstr(user_pathname, pathname, sizeof(pathname), &n);
  if (error < 0)
    return error;

  kprintf("[syscall] open(%s, %d, %d)\n", pathname, flags, mode);

  int fd;
  error = do_open(td, pathname, flags, mode, &fd);
  if (error)
    return -error;
  return fd;
}

int sys_close(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];

  kprintf("[syscall] close(%d)\n", fd);

  return -do_close(td, fd);
}

int sys_read(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  char buf[256];
  count = min(count, 256);

  kprintf("[syscall] read(%d, %p, %zu)\n", fd, ubuf, count);

  uio_t uio;
  iovec_t iov;
  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_user_vm_map();
  iov.iov_base = buf;
  iov.iov_len = count;
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_resid = count;
  uio.uio_offset = 0;

  int error = do_read(td, fd, &uio);
  if (error)
    return -error;
  int read = count - uio.uio_resid;
  error = copyout(buf, ubuf, read);
  if (error < 0)
    return error;
  return read;
}

int sys_write(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  log("sys_write(%d, %p, %zu)", fd, ubuf, count);

  /* Copyin buf */
  char buf[256];
  count = min(count, 256);
  int error = copyin(ubuf, buf, sizeof(buf));
  if (error < 0)
    return error;

  uio_t uio;
  iovec_t iov;
  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_user_vm_map();
  iov.iov_base = buf;
  iov.iov_len = count;
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_resid = count;
  uio.uio_offset = 0;

  error = do_write(td, fd, &uio);
  if (error)
    return -error;
  return count - uio.uio_resid;
}
