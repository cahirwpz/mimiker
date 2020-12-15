#define KL_LOG KL_FILESYS
#include <sys/klog.h>
#include <sys/devfs.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/linker_set.h>
#include <sys/dirent.h>
#include <sys/vfs.h>
#include <sys/stat.h>

typedef struct devfs_node devfs_node_t;
typedef TAILQ_HEAD(, devfs_node) devfs_node_list_t;

struct devfs_node {
  TAILQ_ENTRY(devfs_node) dn_link;
  char *dn_name;
  ino_t dn_ino;
  nlink_t dn_nlinks;
  mode_t dn_mode;
  uid_t dn_uid;
  gid_t dn_gid;
  void *dn_data;
  vnode_t *dn_vnode;
  devfs_node_t *dn_parent;
  devfs_node_list_t dn_children;
};

static KMALLOC_DEFINE(M_DEVFS, "devfs");

/* device filesystem state structure */
typedef struct devfs_mount {
  mtx_t lock;
  ino_t next_ino;
  devfs_node_t root;
} devfs_mount_t;

static devfs_mount_t devfs = {
  .lock = MTX_INITIALIZER(0), .next_ino = 2, .root = {}};
static vnode_lookup_t devfs_vop_lookup;
static vnode_readdir_t devfs_vop_readdir;
static vnode_getattr_t devfs_vop_getattr;
static vnodeops_t devfs_vnodeops = {.v_lookup = devfs_vop_lookup,
                                    .v_readdir = devfs_vop_readdir,
                                    .v_getattr = devfs_vop_getattr,
                                    .v_open = vnode_open_generic};

static inline devfs_node_t *vn2dn(vnode_t *v) {
  return (devfs_node_t *)v->v_data;
}

static devfs_node_t *devfs_find_child(devfs_node_t *parent,
                                      const componentname_t *cn) {
  assert(mtx_owned(&devfs.lock));

  if (parent == NULL)
    parent = &devfs.root;

  devfs_node_t *dn;
  TAILQ_FOREACH (dn, &parent->dn_children, dn_link)
    if (componentname_equal(cn, dn->dn_name))
      return dn;

  if (componentname_equal(cn, ".."))
    return (parent->dn_parent != NULL ? parent->dn_parent : parent);
  else if (componentname_equal(cn, "."))
    return parent;

  return NULL;
}

static int devfs_add_entry(devfs_node_t *parent, const char *name,
                           devfs_node_t **dnp) {
  assert(mtx_owned(&devfs.lock));

  if (parent == NULL)
    parent = &devfs.root;
  if (parent->dn_vnode->v_type != V_DIR)
    return ENOTDIR;

  if (devfs_find_child(parent, &COMPONENTNAME(name)))
    return EEXIST;

  devfs_node_t *dn = kmalloc(M_DEVFS, sizeof(devfs_node_t), M_ZERO);
  dn->dn_ino = ++devfs.next_ino;
  dn->dn_uid = 0;
  dn->dn_gid = 0;
  dn->dn_nlinks = 1;
  dn->dn_parent = parent;
  dn->dn_name = kstrndup(M_STR, name, DEVFS_NAME_MAX);
  TAILQ_INSERT_TAIL(&parent->dn_children, dn, dn_link);
  parent->dn_nlinks++;
  *dnp = dn;
  return 0;
}

static int devfs_vop_getattr(vnode_t *v, vattr_t *va) {
  devfs_node_t *dn = vn2dn(v);
  memset(va, 0, sizeof(vattr_t));
  va->va_mode = dn->dn_mode;
  va->va_uid = dn->dn_uid;
  va->va_gid = dn->dn_gid;
  va->va_nlink = dn->dn_nlinks;
  va->va_ino = dn->dn_ino;
  /* XXX we have only devices which should have size 0 */
  va->va_size = 0;
  return 0;
}

static void devfs_add_default_vops(vnodeops_t *vops) {
  if (vops->v_open == NULL)
    vops->v_open = vnode_open_generic;

  if (vops->v_access == NULL)
    vops->v_access = vnode_access_generic;

  if (vops->v_getattr == NULL)
    vops->v_getattr = devfs_vop_getattr;
}

int devfs_makedev(devfs_node_t *parent, const char *name, vnodeops_t *vops,
                  void *data, vnode_t **vnode_p) {
  SCOPED_MTX_LOCK(&devfs.lock);

  devfs_node_t *dn;
  int error = devfs_add_entry(parent, name, &dn);
  if (error)
    return error;

  devfs_add_default_vops(vops);
  vnodeops_init(vops);

  dn->dn_vnode = vnode_new(V_DEV, vops, dn);
  dn->dn_data = data;
  dn->dn_mode =
    S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  if (vnode_p) {
    vnode_hold(dn->dn_vnode);
    *vnode_p = dn->dn_vnode;
  }
  klog("devfs: registered '%s' device", name);
  return 0;
}

int devfs_makedir(devfs_node_t *parent, const char *name,
                  devfs_node_t **dir_p) {
  SCOPED_MTX_LOCK(&devfs.lock);

  devfs_node_t *dn;
  int error = devfs_add_entry(parent, name, &dn);
  if (!error) {
    dn->dn_vnode = vnode_new(V_DIR, &devfs_vnodeops, dn);
    dn->dn_data = NULL;
    dn->dn_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    TAILQ_INIT(&dn->dn_children);
    *dir_p = dn;
  }
  return 0;
}

void *devfs_getdata(vnode_t *v) {
  devfs_node_t *dn = vn2dn(v);
  return dn->dn_data;
}

/* We are using a single vnode for each devfs_node instead of allocating a new
   one each time, to simplify things. */
static int devfs_mount(mount_t *m) {
  devfs.root.dn_vnode->v_mount = m;
  m->mnt_data = &devfs;
  return 0;
}

static int devfs_vop_lookup(vnode_t *dv, componentname_t *cn, vnode_t **vp) {
  SCOPED_MTX_LOCK(&devfs.lock);

  if (dv->v_type != V_DIR)
    return ENOTDIR;

  devfs_node_t *dn = devfs_find_child(vn2dn(dv), cn);
  if (!dn)
    return ENOENT;
  *vp = dn->dn_vnode;
  vnode_hold(*vp);
  return 0;
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
  dir->d_type = vt2dt(node->dn_vnode->v_type);
  memcpy(dir->d_name, name, dir->d_namlen + 1);
}

static readdir_ops_t devfs_readdir_ops = {
  .next = devfs_dirent_next,
  .namlen_of = devfs_dirent_namlen,
  .convert = devfs_to_dirent,
};

static int devfs_vop_readdir(vnode_t *v, uio_t *uio) {
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
  dn->dn_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
  dn->dn_uid = 0;
  dn->dn_gid = 0;
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
