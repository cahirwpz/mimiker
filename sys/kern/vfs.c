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

static vnodeops_t vfs_root_ops = {};

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
  mount_t *m = pool_alloc(P_MOUNT, PF_ZERO);

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

/* If `*vp` is a mountpoint, then descend into the root of mounted filesys. */
static int vfs_maybe_descend(vnode_t **vp) {
  vnode_t *v_mntpt;
  vnode_t *v = *vp;
  while (is_mountpoint(v)) {
    int error = VFS_ROOT(v->v_mountedhere, &v_mntpt);
    vnode_unlock(v);
    vnode_drop(v);
    if (error)
      return error;
    v = v_mntpt;
    /* No need to ref this vnode, VFS_ROOT already did it for us. */
    vnode_lock(v);
    *vp = v;
  }
  return 0;
}

/* Call VOP_LOOKUP for a single lookup. */
static int vnr_lookup_once(pathcomponent_t *component, vnode_t *searchdir,
                           vnode_t **foundvn_p) {
  vnode_t *foundvn;
  int error;
  error = VOP_LOOKUP(searchdir, component->pc_nameptr, &foundvn);
  if (error) {
    /*
     * The entry was not found in the directory. This is valid
     * if we are creating an entry and are working
     * on the last component of the path name.
     */
    if (error == ENOENT && component->pc_nameiop == VNR_CREATE &&
        component->pc_flags & VNR_ISLASTPC) {
      foundvn = NULL;
      error = 0;
    }

    goto done;
  }
  /* No need to ref this vnode, VFS_LOOKUP already did it for us. */
  vnode_lock(foundvn);
  vfs_maybe_descend(&foundvn);

done:
  *foundvn_p = foundvn;
  return error;
}

int vfs_nameresolve(const char *path, vnrop_t op, vnrresult_t *vr) {
  /* TODO: This is a simplified implementation, and it does not support many
     required features! These include: relative paths, symlinks, parent dirs */
  int error;
  vnode_t *searchdir, *foundobj;

  if (path[0] == '\0')
    return ENOENT;

  if (strncmp(path, "/", 1) != 0) {
    klog("Relative paths are not supported!");
    return ENOENT;
  }

  /* Skip leading '/' */
  path = path + 1;

  /* Copy path into a local buffer, so that we may process it. */
  size_t n = strlen(path);
  if (n >= PATH_MAX)
    return ENAMETOOLONG;
  char *pathcopy = kmalloc(M_TEMP, PATH_MAX, 0);
  strlcpy(pathcopy, path, PATH_MAX);
  char *pathbuf = pathcopy;
  pathcomponent_t pcmp;
  pcmp.pc_nameiop = op;
  pcmp.pc_flags = 0;

  /* Establish the starting directory for lookup, and lock it. */
  searchdir = vfs_root_vnode;
  if (searchdir->v_type != V_DIR)
    return ENOTDIR;

  vnode_hold(searchdir);
  vnode_lock(searchdir);

  if ((error = vfs_maybe_descend(&searchdir)))
    goto end;

  /* Path was just "/". */
  if (pathbuf[0] == '\0') {
    foundobj = searchdir;
    searchdir = NULL;
    goto endloop;
  }

  while (1) {
    assert(*pathbuf != '/');
    assert(*pathbuf != '\0');

    /* Prepare the next path name component. */
    pcmp.pc_nameptr = strsep(&pathbuf, "/");

    while (pathbuf != NULL && *pathbuf == '/')
      pathbuf++;

    if (pathbuf == NULL || *pathbuf == '\0')
      pcmp.pc_flags |= VNR_ISLASTPC;

    /* Look up the child vnode */
    foundobj = NULL;
    error = vnr_lookup_once(&pcmp, searchdir, &foundobj);
    if (error) {
      vnode_unlock(searchdir);
      vnode_drop(searchdir);
      goto end;
    }
    /* Success with no object returned means we're creating something. */
    if (foundobj == NULL)
      break;

    if (pcmp.pc_flags & VNR_ISLASTPC)
      break;

    /* TODO: Check access to child, to verify we can continue with lookup. */
    vnode_unlock(searchdir);
    vnode_drop(searchdir);
    searchdir = foundobj;
  }

endloop:

  if (foundobj != NULL) {
    vnode_unlock(foundobj);
  }

  // Release the parent directory if is not needed.
  if (op != VNR_CREATE && searchdir != NULL) {
    if (searchdir != foundobj) {
      vnode_unlock(searchdir);
    }
    vnode_drop(searchdir);
    searchdir = NULL;
  }

  vr->vr_vp = foundobj;
  vr->vr_dvp = searchdir;
  error = 0;

end:
  kfree(M_TEMP, pathcopy);
  return error;
}

int vfs_lookup(const char *path, vnode_t **vp) {
  vnrresult_t vr;
  int error = vfs_nameresolve(path, VNR_LOOKUP, &vr);
  *vp = vr.vr_vp;
  return error;
}

int vfs_open(file_t *f, char *pathname, int flags, int mode) {
  vnode_t *v;
  int error = 0;
  error = vfs_lookup(pathname, &v);
  if (error)
    return error;
  int res = VOP_OPEN(v, flags, f);
  /* Drop our reference to v. We received it from vfs_lookup, but we no longer
     need it - file f keeps its own reference to v after open. */
  vnode_drop(v);
  return res;
}

SYSINIT_ADD(vfs, vfs_init, NODEPS);
