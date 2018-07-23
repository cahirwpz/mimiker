#include <devfs.h>
#include <mount.h>
#include <vnode.h>
#include <errno.h>
#include <stdc.h>
#include <mutex.h>
#include <malloc.h>
#include <linker_set.h>
#include <dirent.h>

typedef struct devfs_node devfs_node_t;
typedef TAILQ_HEAD(, devfs_node) devfs_node_list_t;

struct devfs_node {
  TAILQ_ENTRY(devfs_node) dn_link;
  char *dn_name;
  ino_t dn_ino;
  vnode_t *dn_vnode;
  devfs_node_t *dn_parent;
  devfs_node_list_t dn_children;
};

/* device filesystem state structure */
typedef struct devfs_mount {
  mtx_t lock;
  ino_t next_ino;
  devfs_node_t root;
} devfs_mount_t;

static devfs_mount_t devfs = {
  .lock = MTX_INITIALIZER(MTX_RECURSE), .next_ino = 2, .root = {}};
static vnode_lookup_t devfs_vop_lookup;
static vnode_readdir_t devfs_vop_readdir;
static vnodeops_t devfs_vnodeops = {.v_lookup = devfs_vop_lookup,
                                    .v_readdir = devfs_vop_readdir,
                                    .v_open = vnode_open_generic};

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
    dn->dn_ino = ++devfs.next_ino;
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
  dn->dn_parent = parent;
  TAILQ_INIT(&dn->dn_children);

  WITH_MTX_LOCK (&devfs.lock) {
    dn->dn_ino = ++devfs.next_ino;
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
  vnode_hold(*vp);
  return 0;
}

static inline devfs_node_t *vn2dn(vnode_t *v) {
  return (devfs_node_t *)v->v_data;
}

static void *devfs_dirent_next(vnode_t *v, void *it) {
  assert(it != NULL);
  if (it == DIRENT_DOT)
    return DIRENT_DOTDOT;
  if (it == DIRENT_DOTDOT)
    return TAILQ_FIRST(&vn2dn(v)->dn_children);
  return TAILQ_NEXT((devfs_node_t *)it, dn_link);
}

static size_t devfs_dirent_namlen(vnode_t *v, void *it) {
  assert(it != NULL);
  if (it == DIRENT_DOT)
    return 1;
  if (it == DIRENT_DOTDOT)
    return 2;
  return strlen(((devfs_node_t *)it)->dn_name);
}

static void devfs_to_dirent(vnode_t *v, void *it, dirent_t *dir) {
  assert(it != NULL);
  devfs_node_t *node;
  const char *name;
  if (it == DIRENT_DOT) {
    node = vn2dn(v);
    name = ".";
  } else if (it == DIRENT_DOTDOT) {
    node = vn2dn(v)->dn_parent;
    name = "..";
  } else {
    node = (devfs_node_t *)it;
    name = node->dn_name;
  }
  dir->d_fileno = node->dn_ino;
  dir->d_type = (node->dn_vnode->v_type == V_DIR) ? DT_DIR : DT_BLK;
  memcpy(dir->d_name, name, dir->d_namlen + 1);
}

static readdir_ops_t devfs_readdir_ops = {
  .next = devfs_dirent_next,
  .namlen_of = devfs_dirent_namlen,
  .convert = devfs_to_dirent,
};

static int devfs_vop_readdir(vnode_t *v, uio_t *uio, void *state) {
  return readdir_generic(v, uio, &devfs_readdir_ops);
}

static int devfs_root(mount_t *m, vnode_t **vp) {
  *vp = devfs.root.dn_vnode;
  vnode_hold(*vp);
  return 0;
}

static int devfs_init(vfsconf_t *vfc) {
  vnodeops_init(&devfs_vnodeops);

  devfs_node_t *dn = &devfs.root;
  dn->dn_vnode = vnode_new(V_DIR, &devfs_vnodeops, dn);
  dn->dn_name = "";
  dn->dn_parent = dn;
  dn->dn_ino = devfs.next_ino;
  TAILQ_INIT(&dn->dn_children);

  /* Prepare some initial devices */
  typedef void devfs_init_func_t(void);
  SET_DECLARE(devfs_init, devfs_init_func_t);
  devfs_init_func_t **ptr;
  SET_FOREACH (ptr, devfs_init)
    (**ptr)();
  return 0;
}

static vfsops_t devfs_vfsops = {
  .vfs_mount = devfs_mount, .vfs_root = devfs_root, .vfs_init = devfs_init};

static vfsconf_t devfs_conf = {.vfc_name = "devfs",
                               .vfc_vfsops = &devfs_vfsops};

SET_ENTRY(vfsconf, devfs_conf);
