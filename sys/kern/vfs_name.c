#define KL_LOG KL_VFS
#include <sys/klog.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/libkern.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <sys/proc.h>

/* Path name component flags.
 * Used by VFS name resolver to represent its' internal state. */
#define CN_ISLAST 0x00000001     /* this is last component of pathname */
#define CN_REQUIREDIR 0x00000002 /* must be a directory */

bool componentname_equal(const componentname_t *cn, const char *name) {
  if (strlen(name) != cn->cn_namelen)
    return false;
  return strncmp(name, cn->cn_nameptr, cn->cn_namelen) == 0;
}

static void cn_fromname(componentname_t *cn, const char *name) {
  cn->cn_nameptr = name;
  /* Look for end of string or component separator. */
  while (*name != '\0' && *name != '/')
    name++;
  cn->cn_namelen = name - cn->cn_nameptr;
}

static const char *cn_namenext(componentname_t *cn) {
  return cn->cn_nameptr + cn->cn_namelen;
}

static char *vs_bufstart(vnrstate_t *vs) {
  return vs->vs_pathbuf + MAXPATHLEN - vs->vs_pathlen;
}

/* Drop leading slashes. */
static void vs_dropslashes(vnrstate_t *vs) {
  while (vs->vs_nextcn[0] == '/') {
    vs->vs_nextcn++;
    vs->vs_pathlen--;
  }
}

static int vs_prepend(vnrstate_t *vs, const char *buf, size_t len) {
  if (len == 0)
    return ENOENT;
  if (len + vs->vs_pathlen > MAXPATHLEN)
    return ENAMETOOLONG;

  /* null-terminator is already included. */
  vs->vs_pathlen += len;
  memcpy(vs_bufstart(vs), buf, len);
  vs->vs_nextcn = vs_bufstart(vs);
  return 0;
}

/*
 * vnr_symlink_follow: follow a symlink. We prepend content of the symlink to
 * the remaining path. Note that we store pointer to the next path component, so
 * we need back up over any slashes preceding that component that we skipped. If
 * new path starts with '/', we must retry lookup from the root vnode.
 */
static int vnr_symlink_follow(vnrstate_t *vs, vnode_t *searchdir,
                              vnode_t *foundvn, vnode_t **new_searchdirp) {
  componentname_t *cn = &vs->vs_cn;
  int error;

  /* Back up over any slashes that we skipped, as we will need them again. */
  vs->vs_pathlen += vs->vs_nextcn - cn_namenext(cn);
  vs->vs_nextcn = cn_namenext(cn);

  if (vs->vs_loopcnt++ >= MAXSYMLINKS)
    return ELOOP;

  char *pathbuf = kmalloc(M_TEMP, MAXPATHLEN, 0);
  uio_t uio = UIO_SINGLE_KERNEL(UIO_READ, 0, pathbuf, MAXPATHLEN);

  if ((error = VOP_READLINK(foundvn, &uio)))
    goto end;

  if ((error = vs_prepend(vs, pathbuf, MAXPATHLEN - uio.uio_resid)))
    goto end;

  /* Check if root directory should replace current directory. */
  if (vs->vs_nextcn[0] == '/') {
    vnode_put(searchdir);
    searchdir = vfs_root_vnode;
    vnode_get(searchdir);
    vfs_maybe_descend(&searchdir);
    vs_dropslashes(vs);
  }

  *new_searchdirp = searchdir;

end:
  kfree(M_TEMP, pathbuf);
  return error;
}

/* Call VOP_LOOKUP for a single lookup. */
static int vnr_lookup_once(vnrstate_t *vs, vnode_t *searchdir,
                           vnode_t **foundvn_p) {
  vnode_t *foundvn;
  componentname_t *cn = &vs->vs_cn;

  if (componentname_equal(cn, ".."))
    vfs_maybe_ascend(&searchdir);

  int error = VOP_LOOKUP(searchdir, cn, &foundvn);
  if (error) {
    /*
     * The entry was not found in the directory. This is valid
     * if we are creating an entry and are working
     * on the last component of the path name.
     */
    if (error == ENOENT && vs->vs_op == VNR_CREATE &&
        cn->cn_flags & CN_ISLAST) {
      vnode_unlock(searchdir);
      foundvn = NULL;
      error = 0;
    }
  } else {
    /* No need to ref this vnode, VOP_LOOKUP already did it for us. */
    vnode_unlock(searchdir);
    vnode_lock(foundvn);
    vfs_maybe_descend(&foundvn);
  }

  *foundvn_p = foundvn;
  return error;
}

static void vnr_parse_component(vnrstate_t *vs) {
  componentname_t *cn = &vs->vs_cn;

  cn_fromname(cn, vs->vs_nextcn);

  /*
   * If this component is followed by a slash, then remember that
   * this component must be a directory.
   */
  const char *name = cn_namenext(cn);

  if (*name == '/') {
    while (*name == '/')
      name++;
    cn->cn_flags |= CN_REQUIREDIR;
  } else {
    cn->cn_flags &= ~CN_REQUIREDIR;
  }

  /* Last component? */
  if (*name == '\0')
    cn->cn_flags |= CN_ISLAST;
  else
    cn->cn_flags &= ~CN_ISLAST;

  vs->vs_pathlen -= name - vs->vs_nextcn;
  vs->vs_nextcn = name;
}

static int do_nameresolve(vnrstate_t *vs) {
  vnode_t *searchdir, *parentdir = NULL;
  componentname_t *cn = &vs->vs_cn;
  int error;

  if (vs->vs_nextcn[0] == '\0')
    return ENOENT;

  /* Establish the starting directory for lookup and lock it.*/
  if (strncmp(vs->vs_nextcn, "/", 1) != 0) {
    if (vs->vs_atdir != NULL)
      searchdir = vs->vs_atdir;
    else
      searchdir = proc_self()->p_cwd;
  } else {
    searchdir = vfs_root_vnode;
  }

  if (searchdir->v_type != V_DIR)
    return ENOTDIR;

  vnode_get(searchdir);

  vs_dropslashes(vs);

  if ((error = vfs_maybe_descend(&searchdir)))
    return error;

  parentdir = searchdir;
  vnode_hold(parentdir);

  /* Path was just "/". */
  if (vs->vs_nextcn[0] == '\0') {
    vnode_unlock(searchdir);
    vs->vs_dvp = parentdir;
    vs->vs_vp = searchdir;
    return 0;
  }

  for (;;) {
    assert(vs->vs_nextcn[0] != '/');
    assert(vs->vs_nextcn[0] != '\0');

    /* Prepare the next path name component. */
    vnr_parse_component(vs);
    if (vs->vs_cn.cn_namelen > NAME_MAX) {
      vnode_put(searchdir);
      return ENAMETOOLONG;
    }

    vnode_drop(parentdir);
    parentdir = searchdir;
    searchdir = NULL;

    error = vnr_lookup_once(vs, parentdir, &searchdir);
    if (error) {
      vnode_put(parentdir);
      return error;
    }

    /* Success with no object returned means we're creating something. */
    if (searchdir == NULL)
      break;

    if (searchdir->v_type == V_LNK &&
        ((vs->vs_flags & VNR_FOLLOW) || (cn->cn_flags & CN_REQUIREDIR))) {
      vnode_lock(parentdir);
      error = vnr_symlink_follow(vs, parentdir, searchdir, &parentdir);
      vnode_unlock(parentdir);
      vnode_put(searchdir);
      if (error) {
        vnode_drop(parentdir);
        return error;
      }
      searchdir = parentdir;
      vnode_lock(searchdir);

      /*
       * If we followed a symlink to `/' and there
       * are no more components after the symlink,
       * we're done with the loop and what we found
       * is the searchdir.
       */
      if (vs->vs_nextcn[0] == '\0') {
        parentdir = NULL;
        break;
      }

      vnode_hold(parentdir);
      continue;
    }

    if (cn->cn_flags & CN_ISLAST)
      break;
  }

  if (searchdir != NULL && vs->vs_op != VNR_DELETE)
    vnode_unlock(searchdir);

  /* Release the parent directory if is not needed. */
  if (vs->vs_op == VNR_LOOKUP && parentdir != NULL) {
    vnode_drop(parentdir);
    parentdir = NULL;
  }

  /* Users of this function assume that parentdir locked */
  if (parentdir != NULL && parentdir != searchdir)
    vnode_lock(parentdir);

  vs->vs_vp = searchdir;
  vs->vs_dvp = parentdir;
  memcpy(&vs->vs_lastcn, &vs->vs_cn, sizeof(componentname_t));
  return 0;
}

int vnrstate_init(vnrstate_t *vs, vnrop_t op, uint32_t flags,
                  const char *path) {
  vs->vs_atdir = NULL;
  vs->vs_op = op;
  vs->vs_flags = flags;
  vs->vs_path = path;
  vs->vs_pathbuf = NULL;
  if (strlen(path) >= MAXPATHLEN)
    return ENAMETOOLONG;
  vs->vs_pathbuf = kmalloc(M_TEMP, MAXPATHLEN, 0);
  if (!vs->vs_pathbuf)
    return ENOMEM;
  return 0;
}

void vnrstate_destroy(vnrstate_t *vs) {
  kfree(M_TEMP, vs->vs_pathbuf);
}

int vfs_nameresolve(vnrstate_t *vs) {
  vs->vs_pathlen = strlen(vs->vs_path) + 1;
  vs->vs_cn.cn_flags = 0;
  vs->vs_nextcn = vs_bufstart(vs);
  vs->vs_loopcnt = 0;

  memcpy(vs_bufstart(vs), vs->vs_path, vs->vs_pathlen);

  return do_nameresolve(vs);
}

int vfs_namelookup(const char *path, vnode_t **vp) {
  vnrstate_t vs;
  int error;

  if ((error = vnrstate_init(&vs, VNR_LOOKUP, VNR_FOLLOW, path)))
    return error;

  error = vfs_nameresolve(&vs);
  *vp = vs.vs_vp;

  vnrstate_destroy(&vs);
  return error;
}

int vfs_name_in_dir(vnode_t *dv, vnode_t *v, char *buf, size_t *lastp) {
  int error = 0;

  vattr_t va;
  if ((error = VOP_GETATTR(v, &va)))
    return error;

  int offset = 0;
  size_t last = *lastp;
  uio_t uio;

  /* TODO: Should use FS block size here instead of PATH_MAX. */
  dirent_t *dirents = kmalloc(M_TEMP, PATH_MAX, 0);

  for (;;) {
    uio = UIO_SINGLE_KERNEL(UIO_READ, offset, dirents, PATH_MAX);
    if ((error = VOP_READDIR(dv, &uio)))
      goto end;

    intptr_t nread = uio.uio_offset - offset;
    if (nread == 0)
      break;

    /* Look for dirent with matching inode number. */
    for (dirent_t *de = dirents; (void *)de < (void *)dirents + nread;
         de = _DIRENT_NEXT(de)) {
      if (de->d_fileno != va.va_ino)
        continue;
      if (last < de->d_namlen) {
        error = ENAMETOOLONG;
      } else {
        last -= de->d_namlen;
        memcpy(&buf[last], de->d_name, de->d_namlen);
      }
      goto end;
    }
    offset = uio.uio_offset;
  }
  error = ENOENT;

end:
  *lastp = last;
  kfree(M_TEMP, dirents);
  return error;
}
