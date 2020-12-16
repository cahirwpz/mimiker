#define KL_LOG KL_VFS
#include <sys/klog.h>
#include <sys/dirent.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <sys/libkern.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/cred.h>

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
static int vnr_symlink_follow(vnrstate_t *vs, vnode_t **searchdir_p,
                              vnode_t *foundvn) {
  vnode_t *searchdir = *searchdir_p;
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

  *searchdir_p = searchdir;

end:
  kfree(M_TEMP, pathbuf);
  return error;
}

static int vnr_check_lookup(vnode_t *searchdir, cred_t *cred) {
  if (searchdir->v_type != V_DIR)
    return ENOTDIR;

  return VOP_ACCESS(searchdir, VEXEC, cred);
}

/* Call VOP_LOOKUP for a single lookup.
 * searchdir vnode is locked on entry and remains locked on return. */
static int vnr_lookup_once(vnrstate_t *vs, vnode_t **searchdir_p,
                           vnode_t **foundvn_p) {
  componentname_t *cn = &vs->vs_cn;
  vnode_t *searchdir = *searchdir_p;
  vnode_t *foundvn;
  cred_t *cred = vs->vs_cred;
  int error;

  if (componentname_equal(cn, ".."))
    vfs_maybe_ascend(&searchdir);

  if ((error = vnr_check_lookup(searchdir, cred)))
    return error;

  if ((error = VOP_LOOKUP(searchdir, cn, &foundvn))) {
    /*
     * The entry was not found in the directory. This is valid if we are
     * creating an entry and are working on the last component of the path name.
     */
    if (error == ENOENT && vs->vs_op == VNR_CREATE && cn->cn_flags & CN_ISLAST)
      error = 0;

    *foundvn_p = NULL;
    *searchdir_p = searchdir;
    return error;
  }

  /* No need to ref foundvn vnode, VOP_LOOKUP already did it for us. */
  if (searchdir != foundvn)
    vnode_lock(foundvn);

  if (is_mountpoint(foundvn)) {
    bool relock_searchdir = (searchdir == foundvn);
    vfs_maybe_descend(&foundvn);

    /* Searchdir needs to be re-locked since it might be released in
     * vfs_maybe_descend */
    if (relock_searchdir)
      vnode_lock(searchdir);
  }

  *foundvn_p = foundvn;
  *searchdir_p = searchdir;
  return 0;
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
  vnode_t *foundvn = NULL, *searchdir = NULL;
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
  if ((error = vfs_maybe_descend(&searchdir)))
    return error;

  vs_dropslashes(vs);

  for (;;) {
    assert(vs->vs_nextcn[0] != '/');

    /* At the beginning of each iteration searchdir is locked and
     * foundvn is NULL. */

    if (vs->vs_nextcn[0] == '\0') {
      foundvn = searchdir;
      searchdir = NULL;
      break;
    }

    /* Prepare the next path name component. */
    vnr_parse_component(vs);
    if (vs->vs_cn.cn_namelen > NAME_MAX) {
      vnode_put(searchdir);
      return ENAMETOOLONG;
    }

    if ((error = vnr_lookup_once(vs, &searchdir, &foundvn))) {
      vnode_put(searchdir);
      return error;
    }

    /* Success with no object returned means we're creating something. */
    if (foundvn == NULL)
      break;

    if (foundvn->v_type == V_LNK &&
        ((vs->vs_flags & VNR_FOLLOW) || (cn->cn_flags & CN_REQUIREDIR))) {
      error = vnr_symlink_follow(vs, &searchdir, foundvn);
      vnode_put(foundvn);
      foundvn = NULL;
      if (error) {
        vnode_put(searchdir);
        return error;
      }
      /*
       * If we followed a symlink to `/' and there are no more components after
       * the symlink, we're done with the loop and what we found is the foundvn.
       */
      if (vs->vs_nextcn[0] == '\0') {
        foundvn = searchdir;
        searchdir = NULL;
        break;
      }
      continue;
    }

    if (cn->cn_flags & CN_ISLAST)
      break;

    if (searchdir != NULL) {
      if (searchdir == foundvn)
        vnode_drop(searchdir);
      else
        vnode_put(searchdir);
    }

    searchdir = foundvn;
    foundvn = NULL;
  }

  bool lockleaf = (vs->vs_op == VNR_DELETE);
  bool lockparent = (vs->vs_op != VNR_LOOKUP);

  if (foundvn != NULL) {
    /*
     * If the caller requested the parent node (i.e. it's a CREATE, DELETE,
     * or RENAME), and we don't have one (because this is the root directory,
     * or we crossed a mount point), then we must fail.
     */
    if (lockparent &&
        (searchdir == NULL || searchdir->v_mount != foundvn->v_mount)) {
      if (searchdir)
        vnode_put(searchdir);
      vnode_put(foundvn);

      if (vs->vs_op == VNR_CREATE)
        return EEXIST;
      if (vs->vs_op == VNR_DELETE || vs->vs_op == VNR_RENAME)
        return EBUSY;
    }

    if (!lockleaf) {
      if (foundvn != searchdir || !lockparent)
        vnode_unlock(foundvn);
    }
  }

  /* Release the parent directory if is not needed. */
  if (searchdir != NULL && !lockparent) {
    if (searchdir == foundvn)
      vnode_drop(searchdir);
    else
      vnode_put(searchdir);
    searchdir = NULL;
  }

  vs->vs_vp = foundvn;
  vs->vs_dvp = searchdir;
  memcpy(&vs->vs_lastcn, &vs->vs_cn, sizeof(componentname_t));
  return 0;
}

int vnrstate_init(vnrstate_t *vs, vnrop_t op, uint32_t flags, const char *path,
                  cred_t *cred) {
  vs->vs_atdir = NULL;
  vs->vs_op = op;
  vs->vs_flags = flags;
  vs->vs_path = path;
  vs->vs_cred = cred;
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

int vfs_namelookup(const char *path, vnode_t **vp, cred_t *cred) {
  vnrstate_t vs;
  int error;

  if ((error = vnrstate_init(&vs, VNR_LOOKUP, VNR_FOLLOW, path, cred)))
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
