#include <mount.h>
#include <stdc.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <vnode.h>
#include <linker_set.h>

static MALLOC_DEFINE(vfs_pool, "VFS pool");

/* The list of all installed filesystem types */
typedef TAILQ_HEAD(, vfsconf) vfsconf_list_t;
static vfsconf_list_t vfsconf_list = TAILQ_HEAD_INITIALIZER(vfsconf_list);
static mtx_t vfsconf_list_mtx;

/* The list of all mounts mounted */
typedef TAILQ_HEAD(, mount) mount_list_t;
static mount_list_t mount_list = TAILQ_HEAD_INITIALIZER(mount_list);
static mtx_t mount_list_mtx;

/* Default vfs operations */
static vfs_root_t vfs_default_root;
static vfs_statfs_t vfs_default_statfs;
static vfs_vget_t vfs_default_vget;
static vfs_init_t vfs_default_init;

/* Global root vnodes */
vnode_t *vfs_root_vnode;
vnode_t *vfs_root_dev_vnode;

static vnodeops_t vfs_root_ops = {
  .v_lookup = vnode_op_notsup,
  .v_readdir = vnode_op_notsup,
  .v_open = vnode_op_notsup,
  .v_read = vnode_op_notsup,
  .v_write = vnode_op_notsup,
};

static int vfs_register(vfsconf_t *vfc);

void vfs_init() {
  mtx_init(&vfsconf_list_mtx);
  mtx_init(&mount_list_mtx);

  /* TODO: We probably need some fancier allocation, since eventually we should
   * start recycling vnodes */
  kmalloc_init(vfs_pool);
  kmalloc_add_arena(vfs_pool, pm_alloc(2)->vaddr, PAGESIZE * 2);

  vfs_root_vnode = vnode_new(V_DIR, &vfs_root_ops);
  vfs_root_dev_vnode = vnode_new(V_DIR, &vfs_root_ops);

  /* Initialize available filesystem types. */
  SET_DECLARE(vfsconf, vfsconf_t);
  vfsconf_t **ptr;
  SET_FOREACH(ptr, vfsconf) {
    vfs_register(*ptr);
  }
}

vfsconf_t *vfs_get_by_name(const char *name) {
  vfsconf_t *vfc;
  mtx_lock(&vfsconf_list_mtx);
  TAILQ_FOREACH (vfc, &vfsconf_list, vfc_list) {
    if (!strcmp(name, vfc->vfc_name)) {
      mtx_unlock(&vfsconf_list_mtx);
      return vfc;
    }
  }
  mtx_unlock(&vfsconf_list_mtx);
  return NULL;
}

/* Register a file system type */
static int vfs_register(vfsconf_t *vfc) {
  /* Check if this file system type was already registered */
  if (vfs_get_by_name(vfc->vfc_name))
    return EEXIST;

  mtx_lock(&vfsconf_list_mtx);
  TAILQ_INSERT_TAIL(&vfsconf_list, vfc, vfc_list);
  mtx_unlock(&vfsconf_list_mtx);

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
  mount_t *m = kmalloc(vfs_pool, sizeof(mount_t), M_ZERO);

  m->mnt_vfc = vfc;
  m->mnt_vfsops = vfc->vfc_vfsops;
  vfc->vfc_mountcnt++; /* TODO: vfc_mtx? */
  m->mnt_data = NULL;

  m->mnt_vnodecovered = v;

  m->mnt_refcnt = 0;
  mtx_init(&m->mnt_mtx);

  return m;
}

int vfs_domount(vfsconf_t *vfc, vnode_t *v) {
  /* Start by checking whether this vnode can be used for mounting */
  if (v->v_type != V_DIR)
    return ENOTDIR;
  if (v->v_mountedhere != NULL)
    return EBUSY;

  /* TODO: Mark the vnode is in-progress of mounting? See VI_MOUNT in FreeBSD */

  mount_t *m = vfs_mount_alloc(v, vfc);

  /* Mount the filesystem. */
  int error = VFS_MOUNT(m);
  if (error != 0) {
    return error;
  }

  v->v_mountedhere = m;

  mtx_lock(&mount_list_mtx);
  TAILQ_INSERT_TAIL(&mount_list, m, mnt_list);
  mtx_unlock(&mount_list_mtx);

  /* Note that we do not need to ask the new mount for the root vnode! That
     V_DIR vnode which is at the mount point stays in place. The root vnode is
     requested during path lookup process, once we encounter a directory that
     has v_mountedhere set. */

  /* TODO: 'mountcheckdirs', which checks each process in the system, and
     updates their dirs to reflect on the mounted file system... */

  return 0;
}

int vfs_lookup(const char *path, vnode_t **vp) {
  /* TODO: This is a simplified implementation, and it does not support many
     required features! These include: relative paths, symlinks, parent dirs */

  if (path[0] == '\0')
    return EINVAL;

  vnode_t *v;
  if (strncmp(path, "/dev/", 5) == 0) {
    /* Handle the special case of "/dev",
     * since we don't have any filesystem at / (root) yet. */
    v = vfs_root_dev_vnode;
    path = path + 5;
  } else if (strncmp(path, "/", 1) == 0) {
    v = vfs_root_vnode;
    path = path + 1;
  } else {
    log("Relative paths are not supported!");
    return ENOTSUP;
  }

  /* Copy path into a local buffer, so that we may process it. */
  size_t n = strlen(path);
  if (n >= VFS_PATH_MAX)
    return ENAMETOOLONG;
  char pathcopy[VFS_PATH_MAX];
  strlcpy(pathcopy, path, VFS_PATH_MAX);
  char *pathbuf = pathcopy;
  const char *component;

  vnode_ref(v);
  vnode_lock(v);

  while ((component = strsep(&pathbuf, "/")) != NULL) {
    if (component[0] == '\0')
      continue;

    /* If this vnode is a filesystem boundary,
     * request the root vnode of the inner filesystem. */
    if (v->v_mountedhere != NULL) {
      vnode_t *v_mntpt;
      int error = VFS_ROOT(v->v_mountedhere, &v_mntpt);
      vnode_unlock(v);
      vnode_unref(v);
      if (error)
        return error;
      v = v_mntpt;
      vnode_ref(v);
      vnode_lock(v);
    }

    /* Look up the child vnode */
    vnode_t *v_child;
    int error = VOP_LOOKUP(v, component, &v_child);
    vnode_unlock(v);
    vnode_unref(v);
    if (error)
      return error;
    v = v_child;
  }

  vnode_unlock(v);
  *vp = v;

  return 0;
}
