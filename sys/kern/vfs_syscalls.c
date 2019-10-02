#define KL_LOG KL_VFS
#include <sys/klog.h>
#include <sys/filedesc.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/stat.h>

int do_open(proc_t *p, char *pathname, int flags, mode_t mode, int *fd) {
  int error;

  /* Allocate a file structure, but do not install descriptor yet. */
  file_t *f = file_alloc();
  /* Try opening file. Fill the file structure. */
  if ((error = vfs_open(f, pathname, flags, mode)))
    goto fail;
  /* Now install the file in descriptor table. */
  if ((error = fdtab_install_file(p->p_fdtable, f, fd)))
    goto fail;
  return 0;

fail:
  file_destroy(f);
  return error;
}

int do_close(proc_t *p, int fd) {
  return fdtab_close_fd(p->p_fdtable, fd);
}

int do_read(proc_t *p, int fd, uio_t *uio) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, FF_READ, &f)))
    return error;
  uio->uio_offset = f->f_offset;
  error = FOP_READ(f, uio);
  f->f_offset = uio->uio_offset;
  file_drop(f);
  return error;
}

int do_write(proc_t *p, int fd, uio_t *uio) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, FF_WRITE, &f)))
    return error;
  uio->uio_offset = f->f_offset;
  error = FOP_WRITE(f, uio);
  f->f_offset = uio->uio_offset;
  file_drop(f);
  return error;
}

int do_lseek(proc_t *p, int fd, off_t offset, int whence, off_t *newoffp) {
  /* TODO: RW file flag! For now we just file_get_read */
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, 0, &f)))
    return error;
  error = FOP_SEEK(f, offset, whence);
  *newoffp = f->f_offset;
  file_drop(f);
  return error;
}

int do_fstat(proc_t *p, int fd, stat_t *sb) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, FF_READ, &f)))
    return error;
  error = FOP_STAT(f, sb);
  file_drop(f);
  return error;
}

int do_stat(proc_t *p, char *path, stat_t *sb) {
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

int do_dup(proc_t *p, int oldfd, int *newfdp) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, oldfd, 0, &f)))
    return error;
  if ((error = fdtab_install_file(p->p_fdtable, f, newfdp)))
    return error;
  file_drop(f);
  return error;
}

int do_dup2(proc_t *p, int oldfd, int newfd) {
  file_t *f;
  int error;

  if (oldfd == newfd)
    return 0;

  if ((error = fdtab_get_file(p->p_fdtable, oldfd, 0, &f)))
    return error;
  error = fdtab_install_file_at(p->p_fdtable, f, newfd);
  file_drop(f);
  return 0;
}

int do_mount(const char *fs, const char *path) {
  vfsconf_t *vfs;
  vnode_t *v;
  int error;

  if (!(vfs = vfs_get_by_name(fs)))
    return EINVAL;
  if ((error = vfs_lookup(path, &v)))
    return error;

  return vfs_domount(vfs, v);
}

int do_getdirentries(proc_t *p, int fd, uio_t *uio, off_t *basep) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, FF_READ, &f)))
    return error;

  uio->uio_offset = f->f_offset;
  error = VOP_READDIR(f->f_vnode, uio, f->f_data);
  f->f_offset = uio->uio_offset;
  *basep = f->f_offset;
  file_drop(f);
  return error;
}

int do_unlink(proc_t *p, char *path) {
  return ENOTSUP;
}

int do_mkdir(proc_t *p, char *path, mode_t mode) {
  return ENOTSUP;
}

int do_rmdir(proc_t *p, char *path) {
  return ENOTSUP;
}

int do_access(proc_t *p, char *path, int amode) {
  int error;

  /* Check if access mode argument is valid. */
  if (amode & ~(R_OK | W_OK | X_OK))
    return EINVAL;

  vnode_t *v;
  if ((error = vfs_lookup(path, &v)))
    return error;
  error = VOP_ACCESS(v, amode);
  vnode_drop(v);
  return error;
}
