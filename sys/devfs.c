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

  return 0;
}

static int devfs_root_lookup(vnode_t *dir, const char *name, vnode_t **res) {
  assert(dir == MOUNT2DEVFS(dir->v_mount)->dfsm_root_vnode);
  /* TODO: Implement. */
  return ENOENT;
}

static int devfs_root_readdir(vnode_t *dir, uio_t *uio) {
  /* TODO: Implement. */
  return ENOTSUP;
}

static int devfs_root(mount_t *m, vnode_t **v) {
  *v = MOUNT2DEVFS(m)->dfsm_root_vnode;
  return 0;
}

void devfs_init() {
  kmalloc_init(devfs_pool);
  kmalloc_add_arena(devfs_pool, pm_alloc(1)->vaddr, PAGESIZE);

  vfs_register(&devfs_conf);

  /* TODO: At some point, mount devfs at /dev. Or maybe at / first, to simplify
   * things for now. */
}
