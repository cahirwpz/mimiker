#define KL_LOG KL_FILESYS
#include <sys/klog.h>
#include <sys/tmpfs.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/libkern.h>
#include <sys/mutex.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/linker_set.h>
#include <sys/dirent.h>

static POOL_DEFINE(P_TMPFS_NODE, "tmpfs node", sizeof(tmpfs_node_t));
static POOL_DEFINE(P_TMPFS_DIRENT, "tmpfs dirent", sizeof(tmpfs_dirent_t));

/* TODO This is a temporary solution. There should be dedicated allocator for mount points. */
static bool tmpfs_mounted;
static tmpfs_mount_t tmpfs;

/* Functions to convert VFS structures to tmpfs internal ones. */
static __inline tmpfs_mount_t *VFS_TO_TMPFS(mount_t *mp) {
  return (tmpfs_mount_t *)mp->mnt_data;
}

static __inline tmpfs_node_t *VFS_TO_TMPFS_NODE(vnode_t *vp) {
  return (tmpfs_node_t *)vp->v_data;
} 

/* Prototypes for internal routines. */
static int tmpfs_init_vnode(vnode_t *vp, tmpfs_node_t *tp);
static int tmpfs_new_node(mount_t *mp, tmpfs_node_t **tp, vnodetype_t ntype);
static int tmpfs_free_inode(tmpfs_mount_t *tmp, tmpfs_node_t *tp);
static int tmpfs_construct_new_node(vnode_t *dvp, vnode_t **vp, vnodetype_t ntype, const char *name);
static int tmpfs_get_node(mount_t *mp, tmpfs_node_t *tp, vnode_t **vp);
static void tmpfs_alloc_dirent(const char *name, tmpfs_dirent_t **de);
static tmpfs_dirent_t *tmpfs_dir_lookup(tmpfs_node_t *dnode, const char *name);

static vnode_lookup_t  tmpfs_vop_lookup;
static vnode_readdir_t tmpfs_vop_readdir;
static vnode_close_t   tmpfs_vop_close;
static vnode_read_t    tmpfs_vop_read;
static vnode_write_t   tmpfs_vop_write;
static vnode_seek_t    tmpfs_vop_seek;
static vnode_getattr_t tmpfs_vop_getattr;
static vnode_create_t  tmpfs_vop_create;
static vnode_remove_t  tmpfs_vop_remove;
static vnode_mkdir_t   tmpfs_vop_mkdir;
static vnode_rmdir_t   tmpfs_vop_rmdir;
static vnode_reclaim_t tmpfs_vop_reclaim;

static vnodeops_t tmpfs_vnodeops = {
  .v_lookup  = tmpfs_vop_lookup,
  .v_readdir = tmpfs_vop_readdir,
  .v_open    = vnode_open_generic,
  .v_close   = tmpfs_vop_close,
  .v_read    = tmpfs_vop_read,
  .v_write   = tmpfs_vop_write,
  .v_seek    = tmpfs_vop_seek,
  .v_getattr = tmpfs_vop_getattr,
  .v_create  = tmpfs_vop_create,
  .v_remove  = tmpfs_vop_remove,
  .v_mkdir   = tmpfs_vop_mkdir,
  .v_rmdir   = tmpfs_vop_rmdir,
  .v_access  = vnode_access_generic,
  .v_reclaim = tmpfs_vop_reclaim
};

/* tmpfs vnode operations */

static int tmpfs_vop_lookup(vnode_t *dv, const char *name, vnode_t **vp) {
  mount_t *mp = dv->v_mount;
  tmpfs_node_t *dnode = VFS_TO_TMPFS_NODE(dv);

  tmpfs_dirent_t *de = tmpfs_dir_lookup(dnode, name);
  if(de == NULL)
    return ENOENT;

  tmpfs_get_node(mp, de->td_node, vp);
  return 0;
}

static int tmpfs_vop_readdir(vnode_t *dv, uio_t *uio, void *state) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_close(vnode_t *v, file_t *fp)  {
  return 0;
}

static int tmpfs_vop_read(vnode_t *v, uio_t *uio)  {
  return EOPNOTSUPP;
}

static int tmpfs_vop_write(vnode_t *v, uio_t *uio) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_seek(vnode_t *v, off_t oldoff, off_t newoff, void *state) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_getattr(vnode_t *v, vattr_t *va) {
  tmpfs_node_t *node = VFS_TO_TMPFS_NODE(v);

  va->va_mode = 0;
  va->va_nlink = node->tn_links;
  va->va_uid = 0;
  va->va_gid = 0;
  va->va_size = 1;

  return 0;
}

static int tmpfs_vop_create(vnode_t *dv, const char *name, vattr_t *va, vnode_t **vp) {
  return tmpfs_construct_new_node(dv, vp, V_REG, name);
}

static int tmpfs_vop_remove(vnode_t *dv, const char *name) {
  return EOPNOTSUPP;
}

static int tmpfs_vop_mkdir(vnode_t *dv, const char *name, vattr_t *va, vnode_t **vp) {
  return tmpfs_construct_new_node(dv, vp, V_DIR, name);
}

static int tmpfs_vop_rmdir(vnode_t *dv, const char *name) {
  return EOPNOTSUPP;
}

/*
 *  tmpfs_vop_reclaim: disassociate inode from vnode and free inode
 *  if there aren't any attached links.
 */
static int tmpfs_vop_reclaim(vnode_t *v) {
  tmpfs_mount_t *tmp = VFS_TO_TMPFS(v->v_mount);
  tmpfs_node_t *node = VFS_TO_TMPFS_NODE(v);

  v->v_data = NULL;
  node->tn_vnode = NULL;

  if(node->tn_links == 0)
    tmpfs_free_inode(tmp, node);

  return 0;
}

/* tmpfs internal routines */

/*
 *  tmpfs_init_vnode: init vnode and associate with existing inode.
 */
static int tmpfs_init_vnode(vnode_t *vp, tmpfs_node_t *tp) {
  tp->tn_vnode = vp;

  vp->v_data = tp;
  vp->v_type = tp->tn_type;
  vp->v_ops = &tmpfs_vnodeops;

  return 0;
}

/*
 *  tmpfs_new_node: create new inode of a specified type and 
 *  attach the vnode.
 */
static int tmpfs_new_node(mount_t *mp, tmpfs_node_t **tp, vnodetype_t ntype) {
  tmpfs_node_t *node = pool_alloc(P_TMPFS_NODE, PF_ZERO);
  node->tn_vnode = NULL;
  node->tn_type = ntype;
  node->tn_links = 0;
  
  switch(node->tn_type) {
    case V_DIR:
      TAILQ_INIT(&node->tn_spec.tn_dir.tn_dir);
      break;
    case V_REG:
      break;
    default:
      panic("bad node type %d", node->tn_type);
  }

  vnode_t *vnode = vnode_new_raw();
  tmpfs_init_vnode(vnode, node);
  vnode->v_mount = mp;

  *tp = node;
  return 0;
}

/* 
 * tmpfs_free_node: remove the inode from a list in the mount point and
 * destroy the inode structures. 
 */
static int tmpfs_free_inode(tmpfs_mount_t *tmp, tmpfs_node_t *tp) {
  pool_free(P_TMPFS_NODE, tp);
  return 0;
}

/*
* tmpfs_construct_new_node: create a new file of specified type and adds it
* into the parent directory.
*/
static int tmpfs_construct_new_node(vnode_t *dvp, vnode_t **vp, vnodetype_t ntype,
                                    const char *name) {
  tmpfs_node_t *dnode = VFS_TO_TMPFS_NODE(dvp);
  tmpfs_dirent_t *de;

  // Allocate a new directory entry for the new file. 
  tmpfs_alloc_dirent(name, &de);

  tmpfs_node_t *node;
  tmpfs_new_node(dvp->v_mount, &node, ntype);

  // Attach directory entry
  node->tn_links++;
  de->td_node = node;
  TAILQ_INSERT_TAIL(&dnode->tn_spec.tn_dir.tn_dir, de, td_entries);

  *vp = node->tn_vnode;
  return 0;
}

/*
 * tmpfs_get_node: get a node with usecnt incremented. 
 */
static int tmpfs_get_node(mount_t *mp, tmpfs_node_t *tp, vnode_t **vp) {
  vnode_t *vnode = tp->tn_vnode;
  if(vnode == NULL) {
    vnode = vnode_new_raw();
    tmpfs_init_vnode(vnode, tp);
    vnode->v_mount = mp;
  } else {
    vnode->v_usecnt++;
  }

  *vp = tp->tn_vnode;
  return 0;
}

/*
 *  tmpfs_alloc_dirent: allocate a new directory entry.
 */
static void tmpfs_alloc_dirent(const char *name, tmpfs_dirent_t **de) {
  tmpfs_dirent_t *dirent = pool_alloc(P_TMPFS_DIRENT, PF_ZERO);

  dirent->td_node = NULL;

  dirent->td_name = kstrndup(M_STR, name, TMPFS_NAME_MAX);
  dirent->td_namelen = strlen(name);

  *de = dirent;
}

static tmpfs_dirent_t *tmpfs_dir_lookup(tmpfs_node_t *dnode, const char *name) {
  tmpfs_dirent_t *de;
  TAILQ_FOREACH (de, &dnode->tn_spec.tn_dir.tn_dir, td_entries)
    if (!strcmp(name, de->td_name))
      return de;
  return NULL;
}

/* tmpfs vfs operations */

static int tmpfs_mount(mount_t *mp) {
  if(tmpfs_mounted)
    panic("Tried to mount more than one tmpfs.");

  /* Allocate the tmpfs mount structure and fill it. */
  tmpfs_mounted = 1;
  tmpfs_mount_t *tmp = &tmpfs;

  tmp->tm_lock = MTX_INITIALIZER(LK_RECURSE);

  mp->mnt_data = tmp;

  /* Allocate the root node. */
  tmpfs_node_t *root;
  tmpfs_new_node(mp, &root, V_DIR);
  root->tn_links++;

  tmp->tm_root = root;
  vnode_drop(root->tn_vnode);
  return 0;
}

static int tmpfs_root(mount_t *mp, vnode_t **vp) {
  tmpfs_mount_t *tmp = VFS_TO_TMPFS(mp);
  tmpfs_get_node(mp, tmp->tm_root, vp);
  return 0;
}

static int tmpfs_init(vfsconf_t *vfc) {
  vnodeops_init(&tmpfs_vnodeops);
  tmpfs_mounted = 0;
  return 0;
}

static vfsops_t tmpfs_vfsops = {
  .vfs_mount = tmpfs_mount,
  .vfs_root  = tmpfs_root,
  .vfs_init  = tmpfs_init
};

static vfsconf_t tmpfs_conf = {.vfc_name   = "tmpfs",
                               .vfc_vfsops = &tmpfs_vfsops};

SET_ENTRY(vfsconf, tmpfs_conf);
