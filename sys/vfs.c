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

void vfs_init() {
  mtx_init(&vfsconf_list_mtx);
  mtx_init(&mount_list_mtx);

  /* TODO: We probably need some fancier allocation, since eventually we should
   * start recycling vnodes */
  kmalloc_init(vfs_pool);
  kmalloc_add_arena(vfs_pool, pm_alloc(2)->vaddr, PAGESIZE);
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

int vfs_lookup(const char *pathname, vnode_t **v) {
  panic("vfs_lookup not yet supported");

  /* The *VERY GENERAL* idea of the process:
   * - Traverse the path, stopping at slashes. Start from root vnode, or
   *   process' working directory.
   * - If the current vnode is not a dir, and you reached the end of the path,
   *   return the vnode.
   * - If the current vnode is a dir, use lookup to get it's child.
   * - If the current vnode is a dir and something is mounted here, get the
   *   mount, and as it for the root.
   * - When you encounter a .. in the path ????? read FreeBSD sources.
   */
}
