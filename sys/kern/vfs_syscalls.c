#define KL_LOG KL_VFS
#include <klog.h>
#include <filedesc.h>
#include <file.h>
#include <mount.h>
#include <stdc.h>
#include <sysent.h>
#include <systm.h>
#include <thread.h>
#include <vfs.h>
#include <vnode.h>
#include <proc.h>
#include <vm_map.h>
#include <queue.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <stat.h>

int do_open(thread_t *td, char *pathname, int flags, mode_t mode, int *fd) {
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
  file_drop(f);
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
  file_drop(f);
  return res;
}

int do_lseek(thread_t *td, int fd, off_t offset, int whence) {
  assert(td->td_proc);
  /* TODO: RW file flag! For now we just file_get_read */
  file_t *f;
  int res = fdtab_get_file(td->td_proc->p_fdtable, fd, 0, &f);
  if (res)
    return res;
  res = FOP_SEEK(f, td, offset, whence);
  file_drop(f);
  return res;
}

int do_fstat(thread_t *td, int fd, stat_t *sb) {
  file_t *f;
  assert(td->td_proc);
  int res = fdtab_get_file(td->td_proc->p_fdtable, fd, FF_READ, &f);
  if (res)
    return res;
  res = FOP_STAT(f, td, sb);
  file_drop(f);
  return res;
}

int do_stat(thread_t *td, char *path, stat_t *sb) {
  vnode_t *v;
  vattr_t va;
  int error;

  if ((error = vfs_lookup(path, &v)))
    return error;
  if ((error = VOP_GETATTR(v, &va)))
    goto fail;

  va_convert(&va, sb);

fail:
  vnode_drop(v);
  return error;
}

int do_dup(thread_t *td, int old) {
  file_t *f;
  assert(td->td_proc);
  int res = fdtab_get_file(td->td_proc->p_fdtable, old, 0, &f);
  if (res)
    return res;
  int new;
  res = fdtab_install_file(td->td_proc->p_fdtable, f, &new);
  file_drop(f);
  return res ? res : new;
}

int do_dup2(thread_t *td, int old, int new) {
  file_t *f;
  assert(td->td_proc);
  if (old == new)
    return 0;
  int res = fdtab_get_file(td->td_proc->p_fdtable, old, 0, &f);
  if (res)
    return res;
  res = fdtab_install_file_at(td->td_proc->p_fdtable, f, new);
  file_drop(f);
  return res ? res : new;
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

int do_getdirentries(thread_t *td, int fd, uio_t *uio, off_t *basep) {
  file_t *f;
  int res = fdtab_get_file(td->td_proc->p_fdtable, fd, FF_READ, &f);
  if (res)
    return res;
  vnode_t *vn = f->f_vnode;
  uio->uio_offset = f->f_offset;
  res = VOP_READDIR(vn, uio, f->f_data);
  f->f_offset = uio->uio_offset;
  *basep = f->f_offset;
  file_drop(f);
  return res;
}

int do_unlink(thread_t *td, char *path) {
  return -ENOTSUP;
}

int do_mkdir(thread_t *td, char *path, mode_t mode) {
  return -ENOTSUP;
}

int do_rmdir(thread_t *td, char *path) {
  return -ENOTSUP;
}

int do_access(thread_t *td, char *path, int amode) {
  int error;

  /* Check if access mode argument is valid. */
  if (amode & ~(R_OK | W_OK | X_OK))
    return -EINVAL;

  vnode_t *v;
  if ((error = vfs_lookup(path, &v)))
    return error;
  error = VOP_ACCESS(v, amode);
  vnode_drop(v);
  return error;
}
