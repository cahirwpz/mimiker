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
#include <sys/libkern.h>
#include <sys/malloc.h>
#include <sys/mount.h>

int do_open(proc_t *p, char *pathname, int flags, mode_t mode, int *fdp) {
  int error;

  /* Allocate a file structure, but do not install descriptor yet. */
  file_t *f = file_alloc();
  /* Try opening file. Fill the file structure. */
  if ((error = vfs_open(f, pathname, flags, mode)))
    goto fail;
  /* Now install the file in descriptor table. */
  if ((error = fdtab_install_file(p->p_fdtable, f, 0, fdp)))
    goto fail;
  /* Set cloexec flag. */
  if (flags & O_CLOEXEC)
    return fd_set_cloexec(p->p_fdtable, *fdp, true);
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
  error = FOP_READ(f, uio);
  file_drop(f);
  return error;
}

int do_write(proc_t *p, int fd, uio_t *uio) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, FF_WRITE, &f)))
    return error;
  error = FOP_WRITE(f, uio);
  file_drop(f);
  return error;
}

int do_lseek(proc_t *p, int fd, off_t offset, int whence, off_t *newoffp) {
  /* TODO: RW file flag! For now we just file_get_read */
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, 0, &f)))
    return error;
  error = FOP_SEEK(f, offset, whence, newoffp);
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

  if ((error = vfs_namelookup(path, &v)))
    return error;
  if ((error = VOP_GETATTR(v, &va)))
    goto fail;

  vattr_convert(&va, sb);

fail:
  vnode_drop(v);
  return error;
}

int do_dup(proc_t *p, int oldfd, int *newfdp) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, oldfd, 0, &f)))
    return error;

  error = fdtab_install_file(p->p_fdtable, f, 0, newfdp);
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
  return error;
}

int do_fcntl(proc_t *p, int fd, int cmd, int arg, int *resp) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, 0, &f)))
    return error;

  /* TODO: Currently only F_DUPFD command is implemented. */
  bool cloexec = false;
  switch (cmd) {
    case F_DUPFD_CLOEXEC:
      cloexec = true;
      /* FALLTHROUGH */
    case F_DUPFD:
      if ((error = fdtab_install_file(p->p_fdtable, f, arg, resp)))
        break;
      error = fd_set_cloexec(p->p_fdtable, fd, cloexec);
      break;

    case F_SETFD:
      if (arg == FD_CLOEXEC)
        cloexec = true;
      error = fd_set_cloexec(p->p_fdtable, fd, cloexec);
      break;

    case F_GETFD:
      error = fd_get_cloexec(p->p_fdtable, fd, resp);
      break;

    default:
      error = EINVAL;
      break;
  }

  file_drop(f);
  return error;
}

int do_mount(const char *fs, const char *path) {
  vfsconf_t *vfs;
  vnode_t *v;
  int error;

  if (!(vfs = vfs_get_by_name(fs)))
    return EINVAL;
  if ((error = vfs_namelookup(path, &v)))
    return error;

  return vfs_domount(vfs, v);
}

int do_getdents(proc_t *p, int fd, uio_t *uio) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, FF_READ, &f)))
    return error;

  uio->uio_offset = f->f_offset;
  error = VOP_READDIR(f->f_vnode, uio);
  f->f_offset = uio->uio_offset;
  file_drop(f);
  return error;
}

int do_unlink(proc_t *p, char *path) {
  int error;
  vnode_t *vn, *dvn;
  componentname_t cn;

  if ((error = vfs_namedelete(path, &dvn, &vn, &cn)))
    return error;

  if (vn->v_type == V_DIR) {
    if (dvn == vn)
      vnode_drop(dvn);
    else
      vnode_put(dvn);
    vnode_put(vn);
    return EPERM;
  }

  error = VOP_REMOVE(dvn, vn, &cn);
  vnode_put(dvn);
  vnode_put(vn);

  return error;
}

int do_mkdir(proc_t *p, char *path, mode_t mode) {
  int error;
  vattr_t va;
  vnode_t *vn, *dvn;
  componentname_t cn;

  if ((error = vfs_namecreate(path, &dvn, &vn, &cn)))
    return error;

  if (vn != NULL) {
    if (vn != dvn)
      vnode_put(dvn);
    else
      vnode_drop(dvn);

    vnode_drop(vn);
    return EEXIST;
  }

  memset(&va, 0, sizeof(vattr_t));
  va.va_mode = S_IFDIR | (mode & ALLPERMS);

  error = VOP_MKDIR(dvn, &cn, &va, &vn);
  if (!error)
    vnode_drop(vn);

  vnode_put(dvn);
  return error;
}

int do_rmdir(proc_t *p, char *path) {
  int error;
  vnode_t *vn, *dvn;
  componentname_t cn;

  if ((error = vfs_namedelete(path, &dvn, &vn, &cn)))
    return error;

  if (vn == dvn)
    error = EINVAL;
  else if (vn->v_type != V_DIR)
    error = ENOTDIR;
  else if (vn->v_mountedhere != NULL)
    error = EBUSY;

  if (error) {
    if (dvn == vn)
      vnode_drop(dvn);
    else
      vnode_put(dvn);
    vnode_put(vn);
    return error;
  }

  error = VOP_RMDIR(dvn, vn, &cn);
  vnode_put(dvn);
  vnode_put(vn);

  return error;
}

int do_access(proc_t *p, char *path, int amode) {
  int error;

  /* Check if access mode argument is valid. */
  if (amode & ~(R_OK | W_OK | X_OK))
    return EINVAL;

  vnode_t *v;
  if ((error = vfs_namelookup(path, &v)))
    return error;
  error = VOP_ACCESS(v, amode);
  vnode_drop(v);
  return error;
}

int do_ioctl(proc_t *p, int fd, u_long cmd, void *data) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, 0, &f)))
    return error;
  error = FOP_IOCTL(f, cmd, data);
  file_drop(f);
  if (error == EPASSTHROUGH)
    error = ENOTTY;
  return error;
}

int do_getcwd(proc_t *p, char *buf, size_t *lastp) {
  assert(*lastp == PATH_MAX);

  vnode_get(p->p_cwd);
  vnode_t *uvp = p->p_cwd;
  vnode_t *lvp = NULL;
  int error = 0;

  /* Last writable position in provided buffer. */
  size_t last = *lastp;

  /* Let's start with terminating NUL. */
  buf[--last] = '\0';

  /* Handle special case for root directory. */
  vfs_maybe_ascend(&uvp);

  if (uvp == vfs_root_vnode) {
    buf[--last] = '/';
    goto end;
  }

  do {
    componentname_t cn = COMPONENTNAME("..");
    if ((error = VOP_LOOKUP(uvp, &cn, &lvp)))
      break;

    if (uvp == lvp) {
      error = ENOENT;
      break;
    }

    if ((error = vfs_name_in_dir(lvp, uvp, buf, &last)))
      break;

    if (last == 0) {
      error = ENAMETOOLONG;
      break;
    }

    buf[--last] = '/'; /* Prepend component separator. */

    vnode_put(uvp);
    vnode_lock(lvp);
    vfs_maybe_ascend(&lvp);
    uvp = lvp;
    lvp = NULL;
  } while (uvp != vfs_root_vnode);

end:
  vnode_put(uvp);
  if (lvp)
    vnode_drop(lvp);
  *lastp = last;
  return error;
}

int do_truncate(proc_t *p, char *path, off_t length) {
  int error;
  vnode_t *vn;

  if ((error = vfs_namelookup(path, &vn)))
    return error;
  vnode_lock(vn);
  if (vn->v_type == V_DIR)
    error = EISDIR;
  else if ((error = VOP_ACCESS(vn, VWRITE))) {
    vattr_t va;
    vattr_null(&va);
    va.va_size = length;
    error = VOP_SETATTR(vn, &va);
  }
  vnode_put(vn);
  return error;
}

int do_ftruncate(proc_t *p, int fd, off_t length) {
  int error;
  file_t *f;

  if ((error = fdtab_get_file(p->p_fdtable, fd, FF_WRITE, &f)))
    return error;

  vnode_t *vn = f->f_vnode;
  vnode_lock(vn);
  if (vn->v_type == V_DIR)
    error = EINVAL;
  else {
    vattr_t va;
    vattr_null(&va);
    va.va_size = length;
    error = VOP_SETATTR(vn, &va);
  }
  vnode_unlock(vn);
  file_drop(f);
  return error;
}
