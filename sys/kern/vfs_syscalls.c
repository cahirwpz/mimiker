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

static int vfs_nameresolveat(proc_t *p, int fdat, vnrstate_t *vs) {
  file_t *f;
  int error;

  if (fdat != AT_FDCWD) {
    if ((error = fdtab_get_file(p->p_fdtable, fdat, FF_READ, &f)))
      return error;
    vs->vs_atdir = f->f_vnode;
  }
  error = vfs_nameresolve(vs);
  if (fdat != AT_FDCWD)
    file_drop(f);

  return error;
}

static int vfs_namelookupat(proc_t *p, int fdat, uint32_t flags,
                            const char *path, vnode_t **vp) {
  vnrstate_t vs;
  int error;

  if ((error = vnrstate_init(&vs, VNR_LOOKUP, flags, path)))
    return error;

  error = vfs_nameresolve(&vs);
  *vp = vs.vs_vp;

  vnrstate_destroy(&vs);
  return error;
}

int do_open(proc_t *p, char *pathname, int flags, mode_t mode, int *fdp) {
  int error;

  /* Allocate a file structure, but do not install descriptor yet. */
  file_t *f = file_alloc();

  /* According to POSIX, the effect when other than permission bits are set in
   * mode is unspecified. Our implementation honors these.
   * https://pubs.opengroup.org/onlinepubs/9699919799/functions/open.html */
  mode = (mode & ~p->p_cmask) & ALLPERMS;

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

int do_fstatat(proc_t *p, int fd, char *path, stat_t *sb, int flag) {
  vnode_t *v;
  vattr_t va;
  int error;
  uint32_t vnrflags = 0;

  if (!(flag & AT_SYMLINK_NOFOLLOW))
    vnrflags |= VNR_FOLLOW;

  if ((error = vfs_namelookupat(p, fd, vnrflags, path, &v)))
    return error;

  if (!(error = VOP_GETATTR(v, &va)))
    vattr_convert(&va, sb);

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
  int flags = 0;
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

    case F_GETFL:
      if (f->f_flags & FF_READ && f->f_flags & FF_WRITE) {
        flags |= O_RDWR;
      } else {
        if (f->f_flags & FF_READ)
          flags |= O_RDONLY;
        if (f->f_flags & FF_WRITE)
          flags |= O_WRONLY;
      }
      if (f->f_flags & FF_APPEND)
        flags |= O_APPEND;
      *resp = flags;
      break;

    case F_SETFL:
      if (arg & O_APPEND)
        flags |= FF_APPEND;
      f->f_flags = flags;
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
  vnrstate_t vs;
  int error;

  if ((error = vnrstate_init(&vs, VNR_DELETE, 0, path)))
    return error;

  if ((error = vfs_nameresolve(&vs)))
    goto fail;

  if (vs.vs_vp->v_type == V_DIR)
    error = EPERM;
  else
    error = VOP_REMOVE(vs.vs_dvp, vs.vs_vp, &vs.vs_lastcn);

  if (vs.vs_dvp == vs.vs_vp)
    vnode_drop(vs.vs_dvp);
  else
    vnode_put(vs.vs_dvp);
  vnode_put(vs.vs_vp);

fail:
  vnrstate_destroy(&vs);
  return error;
}

int do_mkdir(proc_t *p, char *path, mode_t mode) {
  vnrstate_t vs;
  vattr_t va;
  int error;

  if ((error = vnrstate_init(&vs, VNR_CREATE, VNR_FOLLOW, path)))
    return error;

  if ((error = vfs_nameresolve(&vs)))
    goto fail;

  if (vs.vs_vp != NULL) {
    if (vs.vs_vp != vs.vs_dvp)
      vnode_put(vs.vs_dvp);
    else
      vnode_drop(vs.vs_dvp);

    vnode_drop(vs.vs_vp);
    error = EEXIST;
    goto fail;
  }

  memset(&va, 0, sizeof(vattr_t));
  /* We discard all bits but permission bits, since it is
   * implementation-defined.
   * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mkdir.html */
  va.va_mode = S_IFDIR | ((mode & ACCESSPERMS) & ~p->p_cmask);

  error = VOP_MKDIR(vs.vs_dvp, &vs.vs_lastcn, &va, &vs.vs_vp);
  if (!error)
    vnode_drop(vs.vs_vp);

  vnode_put(vs.vs_dvp);

fail:
  vnrstate_destroy(&vs);
  return error;
}

int do_rmdir(proc_t *p, char *path) {
  vnrstate_t vs;
  int error;

  if ((error = vnrstate_init(&vs, VNR_DELETE, VNR_FOLLOW, path)))
    return error;

  if ((error = vfs_nameresolve(&vs)))
    goto fail;

  if (vs.vs_vp == vs.vs_dvp)
    error = EINVAL;
  else if (vs.vs_vp->v_type != V_DIR)
    error = ENOTDIR;
  else if (vs.vs_vp->v_mountedhere != NULL)
    error = EBUSY;

  if (!error)
    error = VOP_RMDIR(vs.vs_dvp, vs.vs_vp, &vs.vs_lastcn);

  if (vs.vs_dvp == vs.vs_vp)
    vnode_drop(vs.vs_dvp);
  else
    vnode_put(vs.vs_dvp);
  vnode_put(vs.vs_vp);

fail:
  vnrstate_destroy(&vs);
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
  if (vn->v_type == V_DIR) {
    error = EINVAL;
  } else {
    vattr_t va;
    vattr_null(&va);
    va.va_size = length;
    error = VOP_SETATTR(vn, &va);
  }
  vnode_unlock(vn);
  file_drop(f);
  return error;
}

ssize_t do_readlinkat(proc_t *p, int fd, char *path, uio_t *uio) {
  vnode_t *v;
  int error;

  if ((error = vfs_namelookupat(p, fd, 0, path, &v)))
    return error;

  if (v->v_type != V_LNK)
    error = EINVAL;
  else if (!(error = VOP_ACCESS(v, VREAD)))
    error = VOP_READLINK(v, uio);

  vnode_drop(v);
  return error;
}

int do_fchdir(proc_t *p, int fd) {
  file_t *f;
  int error;

  if ((error = fdtab_get_file(p->p_fdtable, fd, FF_READ, &f)))
    return error;

  vnode_t *v = f->f_vnode;
  if (v->v_type == V_DIR) {
    vnode_hold(v);
    p->p_cwd = v;
    vnode_drop(p->p_cwd);
  } else {
    error = ENOTDIR;
  }

  file_drop(f);
  return error;
}

int do_symlinkat(proc_t *p, char *target, int newdirfd, char *linkpath) {
  vnrstate_t vs;
  vattr_t va;
  int error;

  if ((error = vnrstate_init(&vs, VNR_CREATE, 0, linkpath)))
    return error;

  if ((error = vfs_nameresolveat(p, newdirfd, &vs)))
    goto fail;

  if (vs.vs_vp != NULL) {
    if (vs.vs_vp != vs.vs_dvp)
      vnode_put(vs.vs_dvp);
    else
      vnode_drop(vs.vs_dvp);

    vnode_drop(vs.vs_vp);

    error = EEXIST;
    goto fail;
  }

  memset(&va, 0, sizeof(vattr_t));
  va.va_mode = S_IFLNK | (ACCESSPERMS & ~p->p_cmask);

  error = VOP_SYMLINK(vs.vs_dvp, &vs.vs_lastcn, &va, target, &vs.vs_vp);
  if (!error)
    vnode_drop(vs.vs_vp);
  vnode_put(vs.vs_dvp);

fail:
  vnrstate_destroy(&vs);
  return error;
}

int do_umask(proc_t *p, int newmask, int *oldmaskp) {
  *oldmaskp = p->p_cmask;
  p->p_cmask = newmask & ALLPERMS;
  return 0;
}
