#include <mount.h>
#include <stdc.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <vnode.h>

static MALLOC_DEFINE(vfs_pool, "VFS pool");

/* The list of all installed filesystem types */
TAILQ_HEAD(vfsconf_list_head, vfsconf);
static struct vfsconf_list_head vfsconf_list =
  TAILQ_HEAD_INITIALIZER(vfsconf_list);
static mtx_t vfsconf_list_mtx;

/* The list of all mounts mounted */
TAILQ_HEAD(mount_list_head, mount);
static struct mount_list_head mount_list = TAILQ_HEAD_INITIALIZER(mount_list);
static mtx_t mount_list_mtx;

/* Default vfs operations */
static vfs_root_t vfs_std_root;
static vfs_vget_t vfs_std_vget;
static vfs_init_t vfs_std_init;

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

void vfs_init() {
  mtx_init(&vfsconf_list_mtx);
  mtx_init(&mount_list_mtx);

  /* TODO: We probably need some fancier allocation, since eventually we should
   * start recycling vnodes */
  kmalloc_init(vfs_pool);
  kmalloc_add_arena(vfs_pool, pm_alloc(2)->vaddr, PAGESIZE);

  vfs_root_vnode = vnode_new(V_DIR, &vfs_root_ops);
  vfs_root_dev_vnode = vnode_new(V_DIR, &vfs_root_ops);
}

vfsconf_t *vfs_get_by_name(const char *name) {
  vfsconf_t *vfsp;
  mtx_lock(&vfsconf_list_mtx);
  TAILQ_FOREACH (vfsp, &vfsconf_list, vfc_list) {
    if (!strcmp(name, vfsp->vfc_name)) {
      mtx_unlock(&vfsconf_list_mtx);
      return vfsp;
    }
  }
  mtx_unlock(&vfsconf_list_mtx);
  return NULL;
}

int vfs_register(vfsconf_t *vfc) {

  /* Check if this file system type was already registered */
  if (vfs_get_by_name(vfc->vfc_name))
    return EEXIST;

  mtx_lock(&vfsconf_list_mtx);
  TAILQ_INSERT_TAIL(&vfsconf_list, vfc, vfc_list);
  mtx_unlock(&vfsconf_list_mtx);

  vfc->vfc_mountcount = 0;

  /* Ensure the filesystem provides obligatory operations */
  assert(vfc->vfc_vfsops != NULL);
  assert(vfc->vfc_vfsops->vfs_mount != NULL);

  /* Use defaults for other operations, if not provided. */
  if (vfc->vfc_vfsops->vfs_root == NULL)
    vfc->vfc_vfsops->vfs_root = vfs_std_root;
  if (vfc->vfc_vfsops->vfs_vget == NULL)
    vfc->vfc_vfsops->vfs_vget = vfs_std_vget;
  if (vfc->vfc_vfsops->vfs_init == NULL)
    vfc->vfc_vfsops->vfs_init = vfs_std_init;

  /* Call init function for this vfs... */
  vfc->vfc_vfsops->vfs_init(vfc);

  return 0;
}

int vfs_std_root(mount_t *m, vnode_t **v) {
  return ENOTSUP;
}
int vfs_std_vget(mount_t *m, int ino, vnode_t **v) {
  return ENOTSUP;
}
int vfs_std_init(vfsconf_t *vfc) {
  return 0;
}

mount_t *vfs_mount_alloc(vnode_t *v, vfsconf_t *vfc) {
  mount_t *m = kmalloc(vfs_pool, sizeof(mount_t), M_ZERO);

  m->mnt_vfc = vfc;
  m->mnt_op = vfc->vfc_vfsops;
  vfc->vfc_mountcount++; /* TODO: vfc_mtx? */
  m->mnt_data = NULL;

  m->mnt_vnodecovered = v;

  m->mnt_ref = 0;
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
  int error = vfc->vfc_vfsops->vfs_mount(m);
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

int vfs_lookup(const char *path, vnode_t **v) {

  /* TODO: This is a simplified implementation, and it does not support many
     required features! These include: relative paths, symlinks, parent dirs */

  if (path[0] == '\0')
    return EINVAL;

  vnode_t *root;
  if (strncmp(path, "/dev", 4) == 0) {
    /* Handle the special case of "/dev", since we don't have any filesystem at
     * / yet. */
    root = vfs_root_dev_vnode;
    path = path + 5;
  } else if (strncmp(path, "/", 1) == 0) {
    root = vfs_root_vnode;
    path = path + 1;
  } else {
    log("Relative paths are not supported!");
    return ENOTSUP;
  }

  /* Copy path into a local buffer, so that we may process it. */
  size_t n = strlen(path);
  if (n >= VFS_MAX_PATH_LENGTH)
    return ENAMETOOLONG;
  char pathbuf[VFS_MAX_PATH_LENGTH];
  strlcpy(pathbuf, path, VFS_MAX_PATH_LENGTH);
  char *path2 = pathbuf;

  vnode_t *current = root;
  const char *tok;
  while ((tok = strsep(&path2, "/")) != NULL) {
    if (tok[0] == '\0')
      continue;
    /* If this vnode is a filesystem boundary, request the root vnode of the
       inner filesystem. */
    if (current->v_mountedhere != NULL) {
      mount_t *m = current->v_mountedhere;
      int error = m->mnt_op->vfs_root(m, &current);
      if (error != 0)
        return error;
    }
    /* Look up the child vnode */
    int error = current->v_ops->v_lookup(current, tok, &current);
    if (error)
      return error;
  }

  *v = current;
  return 0;
}
