#include <tmpfs.h>
#include <mount.h>
#include <vfs.h>
#include <malloc.h>
#include <vnode.h>
#include <errno.h>
#include <common.h>
#include <stdc.h>
#include <dirent.h>
#include <file.h>

static MALLOC_DEFINE(TMPFS_POOL, "tmpfs", 8, 128);

#define TMPFS_BUF_SIZE 1024
#define TMPFS_MAX_NAMELEN 256

static tmpfs_node_t *tmpfs_new_node(tmpfs_node_type type, const char *name) {
  tmpfs_node_t *res = kmalloc(TMPFS_POOL, sizeof(tmpfs_node_t), 0);
  res->name = kstrndup(TMPFS_POOL, name, TMPFS_MAX_NAMELEN);
  res->type = type;

  if (type == T_DIR) {
    TAILQ_INIT(&res->dirdata.head);
  }

  if (type == T_REG) {
    res->data.buf = kmalloc(TMPFS_POOL, TMPFS_BUF_SIZE, 0);
    res->data.size = TMPFS_BUF_SIZE;
  }

  return res;
}

static void tmpfs_delete_node(tmpfs_node_t *node) {
  kfree(TMPFS_POOL, node->name);
  kfree(TMPFS_POOL, node);
}

static void tmpfs_dirnode_insert(tmpfs_dirnode_data_t *dirdata,
                                 tmpfs_node_t *node) {
  TAILQ_INSERT_TAIL(&dirdata->head, node, direntry);
}

static void tmpfs_dirnode_remove(tmpfs_dirnode_data_t *dirdata,
                                 tmpfs_node_t *node) {
  TAILQ_REMOVE(&dirdata->head, node, direntry);
}

static tmpfs_node_t *tmpfs_dirnode_find(tmpfs_dirnode_data_t *dirdata,
                                        const char *name) {
  tmpfs_node_t *it;
  TAILQ_FOREACH (it, &dirdata->head, direntry) {
    if (strcmp(name, it->name) == 0) {
      return it;
    }
  }
  return NULL;
}

int tmpfs_vnode_lookup(vnode_t *dv, const char *name, vnode_t **vp) {
  tmpfs_node_t *dirnode = (tmpfs_node_t *)dv->v_data;
  assert(dirnode->type == T_DIR);
  tmpfs_dirnode_data_t *dirdata = &dirnode->dirdata;
  tmpfs_node_t *node = tmpfs_dirnode_find(dirdata, name);

  if (node) {
    vnodetype_t type = V_REG;
    if (node->type == T_DIR)
      type = V_DIR;
    *vp = vnode_new(type, &tmpfs_ops);
    (*vp)->v_data = (void *)node;
    return 0;
  }

  return -ENOENT;
}

static dirent_t *tmpfs_create_dirent(tmpfs_node_t *node) {
  dirent_t *res = NULL;
  int dirent_reclen = _DIRENT_RECLEN(res, strlen(node->name));
  res = kmalloc(TMPFS_POOL, dirent_reclen, 0);
  res->d_reclen = dirent_reclen;
  res->d_namlen = strlen(node->name);
  if (node->type == T_DIR)
    res->d_type = DT_REG;
  if (node->type == T_REG)
    res->d_type = DT_DIR;
  memcpy(res->d_name, node->name, res->d_namlen + 1);
  return res;
}

typedef struct tmpfs_last_readdir {
  bool first;
  tmpfs_node_t *it;
} tmpfs_last_readdir_t;

int tmpfs_vnode_readdir(vnode_t *dv, uio_t *uio, void *state) {
  tmpfs_node_t *dirnode = (tmpfs_node_t *)dv->v_data;
  off_t offset = uio->uio_offset;
  if (dirnode->type != T_DIR)
    return -ENOTDIR;
  tmpfs_dirnode_data_t *dirdata = &dirnode->dirdata;

  tmpfs_last_readdir_t *last_read = (tmpfs_last_readdir_t *)state;
  tmpfs_node_t *it = last_read->it;
  if (!last_read->first)
    it = TAILQ_FIRST(&dirdata->head);

  for (; it; it = TAILQ_NEXT(it, direntry)) {
    dirent_t *dir = tmpfs_create_dirent(it);
    if (uio->uio_resid >= dir->d_reclen) {
      uiomove(dir, dir->d_reclen, uio);
      last_read->first = 1;
      kfree(TMPFS_POOL, dir);
    } else {
      kfree(TMPFS_POOL, dir);
      break;
    }
  }
  last_read->it = it;
  return uio->uio_offset - offset;
}

int tmpfs_vnode_read(vnode_t *v, uio_t *uio) {
  tmpfs_node_t *node = (tmpfs_node_t *)v->v_data;
  assert(node->type == T_REG);

  tmpfs_node_data_t *data = &node->data;
  return uiomove_frombuf(data->buf, data->size, uio);
}

int tmpfs_vnode_write(vnode_t *v, uio_t *uio) {
  tmpfs_node_t *node = (tmpfs_node_t *)v->v_data;
  assert(node->type == T_REG);

  tmpfs_node_data_t *data = &node->data;
  return uiomove_frombuf(data->buf, data->size, uio);
}

int tmpfs_vnode_seek(vnode_t *v, off_t oldoff, off_t newoff, void *state) {
  /* TODO add support for readdir */
  return vnode_seek_generic(v, oldoff, newoff, state);
}

int tmpfs_vnode_open(vnode_t *v, int mode, file_t *fp) {
  if (v->v_type == V_DIR)
    fp->f_data = kmalloc(TMPFS_POOL, sizeof(tmpfs_last_readdir_t), M_ZERO);
  return vnode_open_generic(v, mode, fp);
}

int tmpfs_vnode_close(vnode_t *v, file_t *fp) {
  if (v->v_type == V_DIR)
    kfree(TMPFS_POOL, fp->f_data);
  return 0;
}

int tmpfs_vnode_getattr(vnode_t *v, vattr_t *va) {
  return 0;
}

int tmpfs_vnode_create(vnode_t *dv, const char *name, vattr_t *va,
                       vnode_t **vp) {
  tmpfs_node_t *dirnode = (tmpfs_node_t *)dv->v_data;
  if (dirnode->type != T_DIR)
    return -ENOTDIR;
  tmpfs_dirnode_data_t *dirdata = &dirnode->dirdata;

  tmpfs_node_t *node = tmpfs_new_node(T_REG, name);
  tmpfs_dirnode_insert(dirdata, node);
  vnode_t *res = vnode_new(T_REG, &tmpfs_ops);
  res->v_data = node;
  *vp = res;

  return 0;
}

int tmpfs_vnode_remove(vnode_t *dv, const char *name) {
  tmpfs_node_t *dirnode = (tmpfs_node_t *)dv->v_data;
  if (dirnode->type != T_DIR)
    return -ENOTDIR;
  tmpfs_dirnode_data_t *dirdata = &dirnode->dirdata;

  tmpfs_node_t *node = tmpfs_dirnode_find(dirdata, name);
  if (node) {
    assert(node->type == T_REG);
    tmpfs_dirnode_remove(dirdata, node);
    tmpfs_delete_node(node);
  }

  return 0;
}

int tmpfs_vnode_mkdir(vnode_t *dv, const char *name, vattr_t *va) {
  tmpfs_node_t *dirnode = (tmpfs_node_t *)dv->v_data;

  if (dirnode->type != T_DIR)
    return -ENOTDIR;

  tmpfs_dirnode_data_t *dirdata = &dirnode->dirdata;

  tmpfs_node_t *node = tmpfs_new_node(T_DIR, name);
  tmpfs_dirnode_insert(dirdata, node);

  return 0;
}

int tmpfs_vnode_rmdir(vnode_t *dv, const char *name) {
  tmpfs_node_t *dirnode = (tmpfs_node_t *)dv->v_data;
  assert(dirnode->type == T_DIR);
  tmpfs_dirnode_data_t *dirdata = &dirnode->dirdata;

  tmpfs_node_t *node = tmpfs_dirnode_find(dirdata, name);
  assert(node->type == T_DIR);
  if (!node) {
    tmpfs_dirnode_remove(dirdata, node);
    tmpfs_delete_node(node);
  }

  return 0;
}

vnodeops_t tmpfs_ops = {.v_lookup = tmpfs_vnode_lookup,
                        .v_readdir = tmpfs_vnode_readdir,
                        .v_open = tmpfs_vnode_open,
                        .v_close = tmpfs_vnode_close,
                        .v_read = tmpfs_vnode_read,
                        .v_write = tmpfs_vnode_write,
                        .v_seek = tmpfs_vnode_seek,
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
  vnode_ref(*v);
  return 0;
}

static int tmpfs_mount(mount_t *m) {
  vnode_t *root = vnode_new(V_DIR, &tmpfs_ops);
  root->v_mount = m;
  m->mnt_data = root;

  tmpfs_node_t *node = tmpfs_new_node(T_DIR, "/");
  root->v_data = node;

  return 0;
}

vfsops_t tmpfs_vfsops = {
  .vfs_mount = tmpfs_mount, .vfs_root = tmpfs_root, .vfs_init = tmpfs_init};

vfsconf_t tmpfs_conf = {.vfc_name = "tmpfs", .vfc_vfsops = &tmpfs_vfsops};

SET_ENTRY(vfsconf, tmpfs_conf);
