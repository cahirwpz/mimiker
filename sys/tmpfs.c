#include <tmpfs.h>
#include <mount.h>
#include <vfs.h>
#include <malloc.h>
#include <vnode.h>

static MALLOC_DEFINE(TMPFS_POOL, "tmpfs", 8, 128);

#define TMPFS_BUF_SIZE 1024

static tmpfs_node_t *tmpfs_new_node(tmpfs_node_type type, const char *name) {
  tmpfs_node_t *res = kmalloc(TMPFS_POOL, sizeof(tmpfs_node_t), 0);
  res->name = name;
  res->type = type;
  if (type == T_DIR) {
    TAILQ_INIT(&res->dirdata.node);
  }

  if (type == T_REG) {
    res->data.buf = kmalloc(TMPFS_POOL, TMPFS_BUF_SIZE, 0);
    res->data.size = TMPFS_BUF_SIZE;
  }

  return res;
}

int tmpfs_vnode_lookup(vnode_t *dv, const char *name, vnode_t **vp) {
  return 0;
}

int tmpfs_vnode_readdir(vnode_t *dv, uio_t *uio, void *state) {
  return 0;
}

int tmpfs_vnode_open(vnode_t *v, int mode, file_t *fp) {
  return 0;
}

int tmpfs_vnode_read(vnode_t *v, uio_t *uio) {
  return 0;
}

int tmpfs_vnode_write(vnode_t *v, uio_t *uio) {
  return 0;
}

int tmpfs_vnode_getattr(vnode_t *v, vattr_t *va) {
  return 0;
}

int tmpfs_vnode_create(vnode_t *dv, const char *name, vnode_t **vp) {
  return 0;
}

int tmpfs_vnode_remove(vnode_t *dv, const char *name) {
  return 0;
}

int tmpfs_vnode_mkdir(vnode_t *v, const char *name, vnode_t **vp) {
  return 0;
}

int tmpfs_vnode_rmdir(vnode_t *v, const char *name) {
  return 0;
}

vnodeops_t tmpfs_ops = {.v_lookup = tmpfs_vnode_lookup,
                               .v_readdir = tmpfs_vnode_readdir,
                               .v_open = tmpfs_vnode_open,
                               .v_read = tmpfs_vnode_read,
                               .v_write = tmpfs_vnode_write,
                               .v_getattr = tmpfs_vnode_getattr,
                               .v_create = tmpfs_vnode_create,
                               .v_remove = tmpfs_vnode_remove,
                               .v_mkdir = tmpfs_vnode_mkdir,
                               .v_rmdir = tmpfs_vnode_rmdir};

static int tmpfs_init(vfsconf_t *vfc) {
  return 0;
}

static int tmpfs_root(mount_t *m, vnode_t **v) {
  *v = (vnode_t *)m->mnt_data;
  return 0;
}

static int tmpfs_mount(mount_t *m) {
  vnode_t *root = vnode_new(V_DIR, &tmpfs_ops);
  root->v_mount = m;

  tmpfs_node_t *node = tmpfs_new_node(T_DIR, "/");
  root->v_data = node;

  return 0;
}
vfsops_t tmpfs_vfsops = {
  .vfs_mount = tmpfs_mount, .vfs_root = tmpfs_root, .vfs_init = tmpfs_init};

vfsconf_t tmpfs_conf = {.vfc_name = "tmpsfs",
                               .vfc_vfsops = &tmpfs_vfsops};

SET_ENTRY(vfsconf, tmpfs_conf);
