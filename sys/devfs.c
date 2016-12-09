#include <devfs.h>
#include <mount.h>
#include <vnode.h>
#include <errno.h>
#include <stdc.h>
#include <malloc.h>

static MALLOC_DEFINE(devfs_pool, "devfs pool");

/* Structure for storage in mnt_data */
typedef struct devfs_mount { vnode_t *dfsm_root_vnode; } devfs_mount_t;
#define MOUNT2DEVFS(m) ((devfs_mount_t *)(m)->mnt_data)

/* Structure for storing installed devices */
typedef struct devfs_installed_device {
  char name[DEVFS_DEVICE_NAME_MAX];
  vnode_t *dev;
  TAILQ_ENTRY(devfs_installed_device) list;
} devfs_installed_device_t;

/* The list of all installed devices */
TAILQ_HEAD(devfs_installed_device_list_head, devfs_installed_device);
static struct devfs_installed_device_list_head devfs_installed_device_list =
  TAILQ_HEAD_INITIALIZER(devfs_installed_device_list);
static mtx_t devfs_installed_device_list_mtx;

static devfs_installed_device_t *devfs_get_dev_by_name(const char *name) {
  devfs_installed_device_t *idev;
  mtx_lock(&devfs_installed_device_list_mtx);
  TAILQ_FOREACH (idev, &devfs_installed_device_list, list) {
    if (!strcmp(name, idev->name)) {
      mtx_unlock(&devfs_installed_device_list_mtx);
      return idev;
    }
  }
  mtx_unlock(&devfs_installed_device_list_mtx);
  return NULL;
}

int devfs_install(const char *name, vnode_t *device) {
  size_t n = strlen(name);
  if (n >= DEVFS_DEVICE_NAME_MAX)
    return ENAMETOOLONG;

  if (devfs_get_dev_by_name(name) != NULL)
    return EEXIST;

  devfs_installed_device_t *idev =
    kmalloc(devfs_pool, sizeof(devfs_installed_device_t), M_ZERO);
  strlcpy(idev->name, name, DEVFS_DEVICE_NAME_MAX);
  idev->dev = device;

  mtx_lock(&devfs_installed_device_list_mtx);
  TAILQ_INSERT_TAIL(&devfs_installed_device_list, idev, list);
  mtx_unlock(&devfs_installed_device_list_mtx);

  return 0;
}

static vfs_mount_t devfs_mount;
static vfs_root_t devfs_root;

static vnode_lookup_t devfs_root_lookup;
static vnode_readdir_t devfs_root_readdir;

static vnodeops_t devfs_root_ops = {
  .v_lookup = devfs_root_lookup,
  .v_readdir = devfs_root_readdir,
  .v_open = vnode_op_notsup,
  .v_read = vnode_op_notsup,
  .v_write = vnode_op_notsup,
};

static vfsops_t devfs_vfsops = {.vfs_mount = devfs_mount,
                                .vfs_root = devfs_root};

static vfsconf_t devfs_conf = {.vfc_name = "devfs",
                               .vfc_vfsops = &devfs_vfsops};

static int devfs_mount(mount_t *m) {
  /* Prepare the root vnode. We'll use a single instead of allocating a new
     vnode each time, because this will suffice for now, and simplifies things a
     lot. */
  vnode_t *root = vnode_new(V_DIR, &devfs_root_ops);
  root->v_mount = m;

  devfs_mount_t *dfm = kmalloc(devfs_pool, sizeof(devfs_mount_t), M_ZERO);
  dfm->dfsm_root_vnode = root;
  m->mnt_data = dfm;

  return 0;
}

static int devfs_root_lookup(vnode_t *dir, const char *name, vnode_t **res) {
  assert(dir == MOUNT2DEVFS(dir->v_mount)->dfsm_root_vnode);

  devfs_installed_device_t *idev = devfs_get_dev_by_name(name);
  if (!idev)
    return ENOENT;

  *res = idev->dev;

  return 0;
}

static int devfs_root_readdir(vnode_t *dir, uio_t *uio) {
  /* TODO: Implement. */
  return ENOTSUP;
}

static int devfs_root(mount_t *m, vnode_t **v) {
  *v = MOUNT2DEVFS(m)->dfsm_root_vnode;
  return 0;
}

extern void init_dev_null();

void devfs_init() {
  kmalloc_init(devfs_pool);
  kmalloc_add_arena(devfs_pool, pm_alloc(1)->vaddr, PAGESIZE);

  mtx_init(&devfs_installed_device_list_mtx);

  vfs_register(&devfs_conf);

  /* Prepare some initial devices */
  init_dev_null();

  /* Mount devfs at /dev. */
  /* TODO: This should actually happen somewhere else in the init process, much
   * later, and is configuration-dependent. */
  vfs_domount(&devfs_conf, vfs_root_dev_vnode);
}
