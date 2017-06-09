#include <devfs.h>
#include <mount.h>
#include <vnode.h>
#include <errno.h>
#include <stdc.h>
#include <mutex.h>
#include <malloc.h>
#include <linker_set.h>

typedef struct devfs_node devfs_node_t;
typedef TAILQ_HEAD(, devfs_node) devfs_node_list_t;

struct devfs_node {
  TAILQ_ENTRY(devfs_node) dn_link;
  char *dn_name;
  vnode_t *dn_vnode;
  devfs_node_list_t dn_children;
};

/* device filesystem state structure */
typedef struct devfs_mount {
  mtx_t lock;
  devfs_node_t root;
} devfs_mount_t;

static devfs_mount_t devfs = {.lock = MUTEX_INITIALIZER(MTX_RECURSE),
                              .root = {}};
static vnode_lookup_t devfs_vop_lookup;
static vnodeops_t devfs_vnodeops = {.v_lookup = devfs_vop_lookup};

static devfs_node_t *devfs_find_child(devfs_node_t *parent, const char *name) {
  SCOPED_MTX_LOCK(&devfs.lock);

  if (parent == NULL)
    parent = &devfs.root;

  devfs_node_t *dn;
  TAILQ_FOREACH (dn, &parent->dn_children, dn_link)
    if (!strcmp(name, dn->dn_name))
      return dn;
  return NULL;
}

int devfs_makedev(devfs_node_t *parent, const char *name, vnodeops_t *vops,
                  void *data) {
  if (parent == NULL)
    parent = &devfs.root;

  if (parent->dn_vnode->v_type != V_DIR)
    return -ENOTDIR;

  devfs_node_t *dn = kmalloc(M_VFS, sizeof(devfs_node_t), M_ZERO);
  dn->dn_name = kstrndup(M_VFS, name, DEVFS_NAME_MAX);
  dn->dn_vnode = vnode_new(V_DEV, vops, data);

  WITH_MTX_LOCK (&devfs.lock) {
    if (devfs_find_child(parent, name))
      return -EEXIST;
    TAILQ_INSERT_TAIL(&parent->dn_children, dn, dn_link);
  }

  return 0;
}

int devfs_makedir(devfs_node_t *parent, const char *name,
                  devfs_node_t **dir_p) {
  if (parent == NULL)
    parent = &devfs.root;

  if (parent->dn_vnode->v_type != V_DIR)
    return -ENOTDIR;

  devfs_node_t *dn = kmalloc(M_VFS, sizeof(devfs_node_t), M_ZERO);
  dn->dn_name = kstrndup(M_VFS, name, DEVFS_NAME_MAX);
  dn->dn_vnode = vnode_new(V_DIR, &devfs_vnodeops, dn);
  TAILQ_INIT(&dn->dn_children);

  WITH_MTX_LOCK (&devfs.lock) {
    if (devfs_find_child(parent, name))
      return -EEXIST;
    TAILQ_INSERT_TAIL(&parent->dn_children, dn, dn_link);
  }

  *dir_p = dn;

  return 0;
}

/* We are using a single vnode for each devfs_node instead of allocating a new
   one each time, to simplify things. */
static int devfs_mount(mount_t *m) {
  devfs.root.dn_vnode->v_mount = m;
  m->mnt_data = &devfs;

  return 0;
}

static int devfs_vop_lookup(vnode_t *dv, const char *name, vnode_t **vp) {
  devfs_node_t *dn;

  if (dv->v_type != V_DIR)
    return -ENOTDIR;
  if (!(dn = devfs_find_child(dv->v_data, name)))
    return -ENOENT;
  *vp = dn->dn_vnode;
  vnode_ref(*vp);
  return 0;
}

static int devfs_root(mount_t *m, vnode_t **vp) {
  *vp = devfs.root.dn_vnode;
  vnode_ref(*vp);
  return 0;
}

static int devfs_init(vfsconf_t *vfc) {
  vnodeops_init(&devfs_vnodeops);

  devfs_node_t *dn = &devfs.root;
  dn->dn_vnode = vnode_new(V_DIR, &devfs_vnodeops, NULL);
  dn->dn_name = "";
  TAILQ_INIT(&dn->dn_children);

  /* Prepare some initial devices */
  typedef void devfs_init_func_t(void);
  SET_DECLARE(devfs_init, devfs_init_func_t);
  devfs_init_func_t **ptr;
  SET_FOREACH(ptr, devfs_init) {
    (**ptr)();
  }
  return 0;
}

static vfsops_t devfs_vfsops = {
  .vfs_mount = devfs_mount, .vfs_root = devfs_root, .vfs_init = devfs_init};

static vfsconf_t devfs_conf = {.vfc_name = "devfs",
                               .vfc_vfsops = &devfs_vfsops};

SET_ENTRY(vfsconf, devfs_conf);
