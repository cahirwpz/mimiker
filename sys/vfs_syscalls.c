#include <filedesc.h>
#include <file.h>
#include <mount.h>
#include <stdc.h>
#include <sysent.h>
#include <systm.h>
#include <thread.h>
#include <vfs_syscalls.h>
#include <vnode.h>
#include <vm_map.h>

int do_open(thread_t *td, char *pathname, int flags, int mode, int *fd) {
  /* Allocate a file structure, but do not install descriptor yet. */
  file_t *f = file_alloc();
  /* Try opening file. Fill the file structure. */
  int error = vfs_open(f, pathname, flags, mode);
  if (error)
    goto fail;
  /* Now install the file in descriptor table. */
  error = fdtab_install_file(td->td_fdtable, f, fd);
  if (error)
    goto fail;

  return 0;

fail:
  file_destroy(f);
  return error;
}

int do_close(thread_t *td, int fd) {
  return fdtab_close_fd(td->td_fdtable, fd);
}

int do_read(thread_t *td, int fd, uio_t *uio) {
  file_t *f;
  int res = fdtab_get_file(td->td_fdtable, fd, FF_READ, &f);
  if (res)
    return res;
  res = FOP_READ(f, td, uio);
  file_unref(f);
  return res;
}

int do_write(thread_t *td, int fd, uio_t *uio) {
  file_t *f;
  int res = fdtab_get_file(td->td_fdtable, fd, FF_WRITE, &f);
  if (res)
    return res;
  res = FOP_WRITE(f, td, uio);
  file_unref(f);
  return res;
}

int do_lseek(thread_t *td, int fd, off_t offset, int whence) {
  /* TODO: Whence! Now we assume whence == SEEK_SET */
  /* TODO: RW file flag! For now we just file_get_read */
  file_t *f;
  int res = fdtab_get_file(td->td_fdtable, fd, FF_READ, &f);
  if (res)
    return res;
  f->f_offset = offset;
  file_unref(f);
  return 0;
}

int do_fstat(thread_t *td, int fd, vattr_t *buf) {
  file_t *f;
  int res = fdtab_get_file(td->td_fdtable, fd, FF_READ, &f);
  if (res)
    return res;
  res = f->f_ops->fo_getattr(f, td, buf);
  file_unref(f);
  return res;
}

int do_readdir(thread_t *td, int fd, uio_t *uio)
{
  file_t *f;
  int res = fdtab_get_file(td->td_fdtable, fd, FF_READ, &f);
  if (res)
    return res;

  vnode_t *vn = f->f_vnode;
  uio->uio_offset = f->f_offset;
  res = VOP_READDIR(vn, uio);

  if (res) {
    f->f_offset += res;
    return res;
  }
  file_unref(f);

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

  log("open(\"%s\", %d, %d)", pathname, flags, mode);

  int fd;
  error = do_open(td, pathname, flags, mode, &fd);
  if (error)
    return error;
  return fd;
}

int sys_close(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];

  log("close(%d)", fd);

  return do_close(td, fd);
}

int sys_read(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  log("sys_read(%d, %p, %zu)", fd, ubuf, count);

  uio_t uio;
  iovec_t iov;
  uio.uio_op = UIO_READ;
  uio.uio_vmspace = get_user_vm_map();
  iov.iov_base = ubuf;
  iov.iov_len = count;
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_resid = count;
  uio.uio_offset = 0;

  int error = do_read(td, fd, &uio);
  if (error)
    return error;
  return count - uio.uio_resid;
}

int sys_write(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  log("sys_write(%d, %p, %zu)", fd, ubuf, count);

  uio_t uio;
  iovec_t iov;
  uio.uio_op = UIO_WRITE;
  uio.uio_vmspace = get_user_vm_map();
  iov.iov_base = ubuf;
  iov.iov_len = count;
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_resid = count;
  uio.uio_offset = 0;

  int error = do_write(td, fd, &uio);
  if (error)
    return error;
  return count - uio.uio_resid;
}

int sys_lseek(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  off_t offset = args->args[1];
  int whence = args->args[2];

  log("sys_lseek(%d, %ld, %d)", fd, offset, whence);

  return do_lseek(td, fd, offset, whence);
}

int sys_fstat(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *buf = (char *)args->args[1];

  log("sys_fstat(%d, %p)", fd, buf);

  vattr_t attr_buf;
  int error = do_fstat(td, fd, &attr_buf);
  if (error)
    return error;
  error = copyout(&attr_buf, buf, sizeof(vattr_t));
  if (error < 0)
    return error;
  return 0;
}

int sys_getdirentries(thread_t *td, syscall_args_t *args)
{
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];
  long *basep = (long*)(uintptr_t)args->args[3];

  log("sys_read(%d, %p, %zu)", fd, ubuf, count);

  uio_t uio;
  iovec_t iov;
  uio.uio_op = UIO_READ;
  uio.uio_vmspace = get_user_vm_map();
  iov.iov_base = ubuf;
  iov.iov_len = count;
  uio.uio_iovcnt = 1;
  uio.uio_iov = &iov;
  uio.uio_resid = count;
  uio.uio_offset = 0;

  int res = do_readdir(td, fd, &uio);
  *basep += count-uio.uio_resid;
  return res;
}
