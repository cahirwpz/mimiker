#define KL_LOG KL_SYSCALL
#include <klog.h>
#include <filedesc.h>
#include <file.h>
#include <mount.h>
#include <stdc.h>
#include <sysent.h>
#include <systm.h>
#include <thread.h>
#include <vfs_syscalls.h>
#include <vnode.h>
#include <proc.h>
#include <vm_map.h>
#include <queue.h>
#include <errno.h>
#include <malloc.h>

int do_open(thread_t *td, char *pathname, int flags, int mode, int *fd) {
  /* Allocate a file structure, but do not install descriptor yet. */
  file_t *f = file_alloc();
  /* Try opening file. Fill the file structure. */
  int error = vfs_open(f, pathname, flags, mode);
  if (error)
    goto fail;
  /* Now install the file in descriptor table. */
  assert(td->td_proc);
  error = fdtab_install_file(td->td_proc->p_fdtable, f, fd);
  if (error)
    goto fail;

  return 0;

fail:
  file_destroy(f);
  return error;
}

int do_close(thread_t *td, int fd) {
  assert(td->td_proc);
  return fdtab_close_fd(td->td_proc->p_fdtable, fd);
}

int do_read(thread_t *td, int fd, uio_t *uio) {
  file_t *f;
  assert(td->td_proc);
  int res = fdtab_get_file(td->td_proc->p_fdtable, fd, FF_READ, &f);
  if (res)
    return res;
  uio->uio_offset = f->f_offset;
  res = FOP_READ(f, td, uio);
  f->f_offset = uio->uio_offset;
  file_unref(f);
  return res;
}

int do_write(thread_t *td, int fd, uio_t *uio) {
  file_t *f;
  assert(td->td_proc);
  int res = fdtab_get_file(td->td_proc->p_fdtable, fd, FF_WRITE, &f);
  if (res)
    return res;
  uio->uio_offset = f->f_offset;
  res = FOP_WRITE(f, td, uio);
  f->f_offset = uio->uio_offset;
  file_unref(f);
  return res;
}

int do_lseek(thread_t *td, int fd, off_t offset, int whence) {
  /* TODO: Whence! Now we assume whence == SEEK_SET */
  /* TODO: RW file flag! For now we just file_get_read */
  file_t *f;
  assert(td->td_proc);
  int res = fdtab_get_file(td->td_proc->p_fdtable, fd, 0, &f);
  if (res)
    return res;
  f->f_offset = offset;
  file_unref(f);
  return 0;
}

int do_fstat(thread_t *td, int fd, vattr_t *buf) {
  file_t *f;
  assert(td->td_proc);
  int res = fdtab_get_file(td->td_proc->p_fdtable, fd, FF_READ, &f);
  if (res)
    return res;
  res = f->f_ops->fo_getattr(f, td, buf);
  file_unref(f);
  return res;
}

int do_mount(thread_t *td, const char *fs, const char *path) {
  vfsconf_t *vfs = vfs_get_by_name(fs);
  if (vfs == NULL)
    return -EINVAL;
  vnode_t *v;
  int error = vfs_lookup(path, &v);
  if (error)
    return error;
  return vfs_domount(vfs, v);
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

  klog("open(\"%s\", %d, %d)", pathname, flags, mode);

  int fd;
  error = do_open(td, pathname, flags, mode, &fd);
  if (error)
    return error;
  return fd;
}

int sys_close(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];

  klog("close(%d)", fd);

  return do_close(td, fd);
}

int sys_read(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  klog("sys_read(%d, %p, %zu)", fd, ubuf, count);

  uio_t uio;
  uio = UIO_SINGLE_USER(UIO_READ, 0, ubuf, count);
  int error = do_read(td, fd, &uio);
  if (error)
    return error;
  return count - uio.uio_resid;
}

int sys_write(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *ubuf = (char *)(uintptr_t)args->args[1];
  size_t count = args->args[2];

  klog("sys_write(%d, %p, %zu)", fd, ubuf, count);

  uio_t uio;
  uio = UIO_SINGLE_USER(UIO_WRITE, 0, ubuf, count);
  int error = do_write(td, fd, &uio);
  if (error)
    return error;
  return count - uio.uio_resid;
}

int sys_lseek(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  off_t offset = args->args[1];
  int whence = args->args[2];

  klog("sys_lseek(%d, %ld, %d)", fd, offset, whence);

  return do_lseek(td, fd, offset, whence);
}

int sys_fstat(thread_t *td, syscall_args_t *args) {
  int fd = args->args[0];
  char *buf = (char *)args->args[1];

  klog("sys_fstat(%d, %p)", fd, buf);

  vattr_t attr_buf;
  int error = do_fstat(td, fd, &attr_buf);
  if (error)
    return error;
  error = copyout(&attr_buf, buf, sizeof(vattr_t));
  if (error < 0)
    return error;
  return 0;
}

int sys_mount(thread_t *td, syscall_args_t *args) {
  char *user_fsysname = (char *)args->args[0];
  char *user_pathname = (char *)args->args[1];

  int error = 0;
  const int PATHSIZE_MAX = 256;
  char *fsysname = kmalloc(M_TEMP, PATHSIZE_MAX, 0);
  char *pathname = kmalloc(M_TEMP, PATHSIZE_MAX, 0);
  size_t n = 0;

  /* Copyout fsysname. */
  error = copyinstr(user_fsysname, fsysname, sizeof(fsysname), &n);
  if (error < 0)
    goto end;
  n = 0;
  /* Copyout pathname. */
  error = copyinstr(user_pathname, pathname, sizeof(pathname), &n);
  if (error < 0)
    goto end;

  klog("mount(\"%s\", \"%s\")", pathname, fsysname);

  error = do_mount(td, fsysname, pathname);
end:
  kfree(M_TEMP, fsysname);
  kfree(M_TEMP, pathname);
  return error;
}
