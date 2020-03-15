#define KL_LOG KL_VFS
#include <sys/klog.h>
#include <sys/mount.h>
#include <sys/libkern.h>
#include <sys/errno.h>
#include <sys/pool.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/linker_set.h>
#include <sys/sysinit.h>
#include <sys/mimiker.h>
#include <sys/proc.h>
#include <sys/stat.h>

/* Path name component flags.
 * Used by VFS name resolver to represent internal state. */
#define VNR_ISLASTPC 0x00000001   /* this is last component of pathname */
#define VNR_REQUIREDIR 0x00000002 /* must be a directory */

/* Internal state for a vnr operation. */
typedef struct {
  /* Arguments to vnr. */
  vnrop_t vs_op;     /* vnr operation type */
  uint32_t vs_flags; /* flags to vfs name resolver */
  vnode_t *vs_atdir; /* startup dir, cwd if null */

  /* Results returned from lookup. */
  vnode_t *vs_vp;  /* vnode of result */
  vnode_t *vs_dvp; /* vnode of parent directory */

  char *vs_pathbuf;  /* pathname buffer */
  size_t vs_pathlen; /* remaining chars in path */
  componentname_t vs_cn;
  const char *vs_nextcn;
  int vs_loopcnt; /* count of symlinks encountered */
} vnrstate_t;

/* TODO: We probably need some fancier allocation, since eventually we should
 * start recycling vnodes */
static POOL_DEFINE(P_MOUNT, "vfs mount points", sizeof(mount_t));

/* The list of all installed filesystem types */
vfsconf_list_t vfsconf_list = TAILQ_HEAD_INITIALIZER(vfsconf_list);
mtx_t vfsconf_list_mtx = MTX_INITIALIZER(0);

/* The list of all mounts mounted */
typedef TAILQ_HEAD(, mount) mount_list_t;
static mount_list_t mount_list = TAILQ_HEAD_INITIALIZER(mount_list);
static mtx_t mount_list_mtx = MTX_INITIALIZER(0);

/* Default vfs operations */
static vfs_root_t vfs_default_root;
static vfs_statfs_t vfs_default_statfs;
static vfs_vget_t vfs_default_vget;
static vfs_init_t vfs_default_init;

/* Global root vnodes */
vnode_t *vfs_root_vnode;

static int vfs_root_vnode_lookup(vnode_t *vdir, componentname_t *cn,
                                 vnode_t **res) {
  if (componentname_equal(cn, "..") || componentname_equal(cn, ".")) {
    vnode_hold(vfs_root_vnode);
    *res = vfs_root_vnode;
    return 0;
  }

  return ENOENT;
}

static vnodeops_t vfs_root_ops = {.v_lookup = vfs_root_vnode_lookup};

static int vfs_register(vfsconf_t *vfc);

static void vfs_init(void) {
  vnodeops_init(&vfs_root_ops);

  vfs_root_vnode = vnode_new(V_DIR, &vfs_root_ops, NULL);

  /* Initialize available filesystem types. */
  SET_DECLARE(vfsconf, vfsconf_t);
  vfsconf_t **ptr;
  SET_FOREACH (ptr, vfsconf)
    vfs_register(*ptr);
}

vfsconf_t *vfs_get_by_name(const char *name) {
  SCOPED_MTX_LOCK(&vfsconf_list_mtx);

  vfsconf_t *vfc;
  TAILQ_FOREACH (vfc, &vfsconf_list, vfc_list)
    if (!strcmp(name, vfc->vfc_name))
      return vfc;
  return NULL;
}

/* Register a file system type */
static int vfs_register(vfsconf_t *vfc) {
  /* Check if this file system type was already registered */
  if (vfs_get_by_name(vfc->vfc_name))
    return EEXIST;

  WITH_MTX_LOCK (&vfsconf_list_mtx)
    TAILQ_INSERT_TAIL(&vfsconf_list, vfc, vfc_list);

  vfc->vfc_mountcnt = 0;

  /* Ensure the filesystem provides obligatory operations */
  assert(vfc->vfc_vfsops != NULL);
  assert(vfc->vfc_vfsops->vfs_mount != NULL);

  /* Use defaults for other operations, if not provided. */
  if (vfc->vfc_vfsops->vfs_root == NULL)
    vfc->vfc_vfsops->vfs_root = vfs_default_root;
  if (vfc->vfc_vfsops->vfs_statfs == NULL)
    vfc->vfc_vfsops->vfs_statfs = vfs_default_statfs;
  if (vfc->vfc_vfsops->vfs_vget == NULL)
    vfc->vfc_vfsops->vfs_vget = vfs_default_vget;
  if (vfc->vfc_vfsops->vfs_init == NULL)
    vfc->vfc_vfsops->vfs_init = vfs_default_init;

  /* Call init function for this vfs... */
  vfc->vfc_vfsops->vfs_init(vfc);

  return 0;
}

static int vfs_default_root(mount_t *m, vnode_t **v) {
  return ENOTSUP;
}

static int vfs_default_statfs(mount_t *m, statfs_t *sb) {
  return ENOTSUP;
}

static int vfs_default_vget(mount_t *m, ino_t ino, vnode_t **v) {
  return ENOTSUP;
}

static int vfs_default_init(vfsconf_t *vfc) {
  return 0;
}

mount_t *vfs_mount_alloc(vnode_t *v, vfsconf_t *vfc) {
  mount_t *m = pool_alloc(P_MOUNT, M_ZERO);

  m->mnt_vfc = vfc;
  m->mnt_vfsops = vfc->vfc_vfsops;
  vfc->vfc_mountcnt++; /* TODO: vfc_mtx? */
  m->mnt_data = NULL;

  m->mnt_vnodecovered = v;

  m->mnt_refcnt = 0;
  mtx_init(&m->mnt_mtx, 0);

  return m;
}

int vfs_domount(vfsconf_t *vfc, vnode_t *v) {
  int error;

  /* Start by checking whether this vnode can be used for mounting */
  if (v->v_type != V_DIR)
    return ENOTDIR;
  if (is_mountpoint(v))
    return EBUSY;

  /* TODO: Mark the vnode is in-progress of mounting? See VI_MOUNT in FreeBSD */

  mount_t *m = vfs_mount_alloc(v, vfc);

  /* Mount the filesystem. */
  if ((error = VFS_MOUNT(m)))
    return error;

  v->v_mountedhere = m;

  WITH_MTX_LOCK (&mount_list_mtx)
    TAILQ_INSERT_TAIL(&mount_list, m, mnt_list);

  /* Note that we do not need to ask the new mount for the root vnode! That
     V_DIR vnode which is at the mount point stays in place. The root vnode is
     requested during path lookup process, once we encounter a directory that
     has v_mountedhere set. */

  /* TODO: 'mountcheckdirs', which checks each process in the system, and
     updates their dirs to reflect on the mounted file system... */

  return 0;
}

void vfs_maybe_ascend(vnode_t **vp) {
  vnode_t *v_covered;
  vnode_t *v = *vp;
  while (vnode_is_mounted(v)) {
    v_covered = v->v_mount->mnt_vnodecovered;
    vnode_get(v_covered);
    vnode_put(v);
    v = v_covered;
  }

  *vp = v;
}

/* If `*vp` is a mountpoint, then descend into the root of mounted filesys. */
static int vfs_maybe_descend(vnode_t **vp) {
  vnode_t *v_mntpt;
  vnode_t *v = *vp;
  while (is_mountpoint(v)) {
    int error = VFS_ROOT(v->v_mountedhere, &v_mntpt);
    vnode_put(v);
    if (error)
      return error;
    v = v_mntpt;
    /* No need to ref this vnode, VFS_ROOT already did it for us. */
    vnode_lock(v);
    *vp = v;
  }
  return 0;
}

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
        cn->cn_flags & VNR_ISLASTPC) {
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
    cn->cn_flags |= VNR_REQUIREDIR;
  } else {
    cn->cn_flags &= ~VNR_REQUIREDIR;
  }

  /* Last component? */
  if (*name == '\0')
    cn->cn_flags |= VNR_ISLASTPC;
  else
    cn->cn_flags &= ~VNR_ISLASTPC;

  vs->vs_pathlen -= name - vs->vs_nextcn;
  vs->vs_nextcn = name;
}

static int vfs_nameresolve(vnrstate_t *vs) {
  /* TODO: This is a simplified implementation, and it does not support many
     required features! These include: symlinks */
  int error;
  vnode_t *searchdir, *parentdir = NULL;
  componentname_t *cn = &vs->vs_cn;

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
    goto end;

  parentdir = searchdir;
  vnode_hold(parentdir);

  /* Path was just "/". */
  if (vs->vs_nextcn[0] == '\0') {
    vnode_unlock(searchdir);
    vs->vs_dvp = parentdir;
    vs->vs_vp = searchdir;
    error = 0;
    goto end;
  }

  for (;;) {
    assert(vs->vs_nextcn[0] != '/');
    assert(vs->vs_nextcn[0] != '\0');

    /* Prepare the next path name component. */
    vnr_parse_component(vs);
    if (vs->vs_cn.cn_namelen > NAME_MAX) {
      error = ENAMETOOLONG;
      vnode_put(searchdir);
      goto end;
    }

    vnode_drop(parentdir);
    parentdir = searchdir;
    searchdir = NULL;

    error = vnr_lookup_once(vs, parentdir, &searchdir);
    if (error) {
      vnode_put(parentdir);
      goto end;
    }

    /* Success with no object returned means we're creating something. */
    if (searchdir == NULL)
      break;

    if (searchdir->v_type == V_LNK &&
        ((vs->vs_flags & VNR_FOLLOW) || (cn->cn_flags & VNR_REQUIREDIR))) {
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

    if (cn->cn_flags & VNR_ISLASTPC)
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
  error = 0;

end:
  return error;
}

static int vnrstate_init(vnrstate_t *vs, vnode_t *atdir, vnrop_t op,
                         uint32_t flags, const char *path) {
  vs->vs_op = op;
  vs->vs_flags = flags;
  vs->vs_atdir = atdir;
  /* length includes null terminator */
  vs->vs_pathlen = strlen(path) + 1;
  if (vs->vs_pathlen > MAXPATHLEN)
    return ENAMETOOLONG;
  vs->vs_pathbuf = kmalloc(M_TEMP, MAXPATHLEN, 0);
  if (!vs->vs_pathbuf)
    return ENOMEM;
  memcpy(vs_bufstart(vs), path, vs->vs_pathlen);

  vs->vs_cn.cn_flags = 0;
  vs->vs_nextcn = vs_bufstart(vs);
  vs->vs_loopcnt = 0;

  return 0;
}

static void vnrstate_destroy(vnrstate_t *vs) {
  kfree(M_TEMP, vs->vs_pathbuf);
}

int vfs_namelookupat(const char *path, vnode_t *atdir, uint32_t flags,
                     vnode_t **vp) {
  vnrstate_t vs;
  int error;
  if ((error = vnrstate_init(&vs, atdir, VNR_LOOKUP, flags, path)))
    return error;
  error = vfs_nameresolve(&vs);
  *vp = vs.vs_vp;

  vnrstate_destroy(&vs);
  return error;
}

int vfs_namecreateat(const char *path, vnode_t *atdir, uint32_t flags,
                     vnode_t **dvp, vnode_t **vp, componentname_t *cn) {
  vnrstate_t vs;
  int error;
  if ((error = vnrstate_init(&vs, atdir, VNR_CREATE, flags, path)))
    return error;
  error = vfs_nameresolve(&vs);
  *dvp = vs.vs_dvp;
  *vp = vs.vs_vp;
  memcpy(cn, &vs.vs_cn, sizeof(componentname_t));

  vnrstate_destroy(&vs);
  return error;
}

int vfs_namedeleteat(const char *path, vnode_t *atdir, uint32_t flags,
                     vnode_t **dvp, vnode_t **vp, componentname_t *cn) {
  vnrstate_t vs;
  int error;
  if ((error = vnrstate_init(&vs, atdir, VNR_DELETE, flags, path)))
    return error;
  error = vfs_nameresolve(&vs);
  *dvp = vs.vs_dvp;
  *vp = vs.vs_vp;
  memcpy(cn, &vs.vs_cn, sizeof(componentname_t));

  vnrstate_destroy(&vs);
  return error;
}

int vfs_open(file_t *f, char *pathname, int flags, int mode) {
  vnode_t *v;
  int error = 0;
  if (flags & O_CREAT) {
    vnode_t *dvp;
    componentname_t cn;
    if ((error = vfs_namecreate(pathname, &dvp, &v, &cn)))
      return error;

    if (v == NULL) {
      vattr_t va;
      vattr_null(&va);
      va.va_mode = S_IFREG | (mode & ALLPERMS);
      error = VOP_CREATE(dvp, &cn, &va, &v);
      vnode_put(dvp);
      if (error)
        return error;
      flags &= ~O_TRUNC;
    } else {
      if (v == dvp)
        vnode_drop(dvp);
      else
        vnode_put(dvp);

      if (mode & O_EXCL)
        error = EEXIST;
      mode &= ~O_CREAT;
    }
  } else {
    if ((error = vfs_namelookup(pathname, &v)))
      return error;
  }

  if (!error && flags & O_TRUNC) {
    vattr_t va;
    vattr_null(&va);
    va.va_size = 0;
    error = VOP_SETATTR(v, &va);
  }

  if (!error)
    error = VOP_OPEN(v, flags, f);

  /* Drop our reference to v. We received it from vfs_namelookup, but we no
     longer need it - file f keeps its own reference to v after open. */
  vnode_drop(v);
  return error;
}

SYSINIT_ADD(vfs, vfs_init, NODEPS);
